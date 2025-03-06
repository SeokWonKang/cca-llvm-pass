#include "Instrumentation/CCAPasses.hpp"
#include "Instrumentation/Utils.hpp"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

using namespace llvm;

namespace llvm {
namespace cca {

#define PRINT_INSTRUCTION(Header, Inst)                                                                                                              \
	outs() << Header;                                                                                                                                \
	(Inst)->print(outs());                                                                                                                           \
	outs() << '\n';

// Struct for Pattern
struct MulSubMulDivPattern {
	BinaryOperator *SubInst;
	BinaryOperator *MulInst0;
	BinaryOperator *MulInst1;
	BinaryOperator *DivInst;
	Value *Bias;
};

// Run
PreservedAnalyses CCAMulSubMulDivPass::run(Function &F, FunctionAnalysisManager &) {
	std::vector<struct MulSubMulDivPattern> MulSubMulDivPatternVec;

	Type *VoidTy = Type::getVoidTy(F.getContext());
	Type *Int32Ty = Type::getInt32Ty(F.getContext());

	// Find MulAdd Patterns
	for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
		for (BasicBlock::iterator BBIter = FuncIter->begin(); BBIter != FuncIter->end(); ++BBIter) {
			if (isa<BinaryOperator>(BBIter) && cast<BinaryOperator>(BBIter)->getOpcode() == Instruction::Sub && BBIter->getType() == Int32Ty) {
				if (F.getName() != "main_kernel1") continue;
				// Find Mul-Sub-Mul-Div Patterns
				// - CASE 1: SUB(MUL0, MUL1(DIV, Bias))
				BinaryOperator *SubInst = cast<BinaryOperator>(BBIter);
				BinaryOperator *MulInst0 = nullptr;
				BinaryOperator *MulInst1 = nullptr;
				BinaryOperator *DivInst = nullptr;
				Value *Bias = nullptr;
				// Check First Operand is Mul Instruction
				if (!isaBO(SubInst->getOperand(0), Instruction::Mul)) continue;
				MulInst0 = cast<BinaryOperator>(SubInst->getOperand(0));
				if (CheckOtherUseExist(MulInst0, SubInst)) continue;
				// Check Second Operand is Mul Instruction
				if (!isaBO(SubInst->getOperand(1), Instruction::Mul)) continue;
				MulInst1 = cast<BinaryOperator>(SubInst->getOperand(1));
				if (CheckOtherUseExist(MulInst1, SubInst)) continue;
				// Check Div Instruction is used in the Second Operand Mul Instruction
				DivInst = GetBinaryOperatorInOperandsInBinaryOperator(MulInst1, Instruction::UDiv, Bias);
				if (DivInst == nullptr || CheckOtherUseExist(DivInst, MulInst1)) continue;

				MulSubMulDivPatternVec.push_back({SubInst, MulInst0, MulInst1, DivInst, Bias});
			}
		}
	}

	FunctionType *MoveInstFT = FunctionType::get(VoidTy, {Int32Ty}, false);
	InlineAsm *Move24InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r24, $0", "r", true);
	InlineAsm *Move25InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r25, $0", "r", true);
	InlineAsm *Move26InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r26, $0", "r", true);
	InlineAsm *Move27InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r27, $0", "r", true);
	InlineAsm *Move28InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r28, $0", "r", true);
	FunctionType *CCAInstFT = FunctionType::get(VoidTy, false);
	InlineAsm *CCAInstIA = InlineAsm::get(CCAInstFT, "#removethiscomment cca 3", "", true);
	FunctionType *Move30InstFT = FunctionType::get(Int32Ty, false);
	InlineAsm *Move30InstIA = InlineAsm::get(Move30InstFT, "#removethiscomment move $0, r30", "=r", true);

	for (auto PatternIter = MulSubMulDivPatternVec.begin(); PatternIter != MulSubMulDivPatternVec.end(); ++PatternIter) {
		BinaryOperator *&SubInst = PatternIter->SubInst;
		BinaryOperator *&MulInst0 = PatternIter->MulInst0;
		BinaryOperator *&MulInst1 = PatternIter->MulInst1;
		BinaryOperator *&DivInst = PatternIter->DivInst;
		Value *&Bias = PatternIter->Bias;
		// CCA Prepare (Implemented as Move instruction)
		CallInst *Move24CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move24InstIA), {MulInst0->getOperand(0)});
		CallInst *Move25CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move25InstIA), {MulInst0->getOperand(1)});
		CallInst *Move26CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move26InstIA), {Bias});
		CallInst *Move27CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move27InstIA), {DivInst->getOperand(0)});
		CallInst *Move28CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move28InstIA), {DivInst->getOperand(1)});
		Move24CallInst->setTailCall(true);
		Move25CallInst->setTailCall(true);
		Move26CallInst->setTailCall(true);
		Move27CallInst->setTailCall(true);
		Move28CallInst->setTailCall(true);
		// CCA Instruction
		CallInst *CCACallInst = CallInst::Create(FunctionCallee(CCAInstFT, CCAInstIA));
		CCACallInst->setTailCall(true);
		// Store Instruction
		CallInst *Move30CallInst = CallInst::Create(FunctionCallee(Move30InstFT, Move30InstIA), {}, "move30inst");
		Move30CallInst->setTailCall(false);
		// Verbose
		outs() << "[CCA:MulSubMulDiv] Found Pattern in Function \"" << F.getName() << "\"\n";
		PRINT_INSTRUCTION(" - source.sub: ", SubInst);
		PRINT_INSTRUCTION(" - source.mul0: ", MulInst0);
		PRINT_INSTRUCTION(" - source.mul1: ", MulInst1);
		PRINT_INSTRUCTION(" - source.div: ", DivInst);
		PRINT_INSTRUCTION(" - output.move24: ", Move24CallInst);
		PRINT_INSTRUCTION(" - output.move25: ", Move25CallInst);
		PRINT_INSTRUCTION(" - output.move26: ", Move26CallInst);
		PRINT_INSTRUCTION(" - output.move27: ", Move27CallInst);
		PRINT_INSTRUCTION(" - output.move28: ", Move28CallInst);
		PRINT_INSTRUCTION(" - output.cca: ", CCACallInst);
		PRINT_INSTRUCTION(" - output.move30: ", Move30CallInst);
		// Insert Instructions
		Move30CallInst->insertAfter(SubInst);
		CCACallInst->insertBefore(Move30CallInst);
		Move28CallInst->insertBefore(CCACallInst);
		Move27CallInst->insertBefore(Move28CallInst);
		Move26CallInst->insertBefore(Move27CallInst);
		Move25CallInst->insertBefore(Move26CallInst);
		Move24CallInst->insertBefore(Move25CallInst);
		// Replace & Erase Instructions
		SubInst->replaceAllUsesWith(Move30CallInst);
		SubInst->eraseFromParent();
		MulInst0->eraseFromParent();
		MulInst1->eraseFromParent();
		DivInst->eraseFromParent();
	}

	return PreservedAnalyses::all();
}

} // namespace cca
} // namespace llvm
