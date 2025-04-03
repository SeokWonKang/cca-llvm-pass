#include "Instrumentation/Fixed/CCAFixedPasses.hpp"
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
struct MulAddDoublePattern {
	BinaryOperator *MulInst0;
	BinaryOperator *MulInst1;
	Value *Bias;
	BinaryOperator *LastAddInst;
	BinaryOperator *NonLastAddInst;
};

// Run
PreservedAnalyses CCAMulAddDoublePass::run(Function &F, FunctionAnalysisManager &) {
	std::vector<struct MulAddDoublePattern> MulAddDoublePatternVec;
	std::vector<StoreInst *> SVecToRemove;

	// Find MulAdd Patterns
	for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
		for (BasicBlock::iterator BBIter = FuncIter->begin(); BBIter != FuncIter->end(); ++BBIter) {
			if (isa<BinaryOperator>(BBIter) && cast<BinaryOperator>(BBIter)->getOpcode() == Instruction::Add) {
				// Find Mul-Add (Double) Patterns
				// - CASE 1: ADD(ADD(MUL0, X), MUL1)
				// - CASE 2: ADD(ADD(MUL0, MUL1), X)
				BinaryOperator *LastAddInst = cast<BinaryOperator>(BBIter);
				BinaryOperator *NonLastAddInst = nullptr;
				Value *NonAddOperand = nullptr;
				BinaryOperator *MulInst0 = nullptr;
				BinaryOperator *MulInst1 = nullptr;
				Value *Bias = nullptr;
				// Find Add-Add Patterns
				NonLastAddInst = GetBinaryOperatorInOperandsInBinaryOperator(LastAddInst, Instruction::Add, NonAddOperand);
				if (NonLastAddInst == nullptr || CheckOtherUseExistWithoutStore(NonLastAddInst, LastAddInst, SVecToRemove)) continue;
				// Check Add Operand is Mul
				if (isa<BinaryOperator>(NonAddOperand) && cast<BinaryOperator>(NonAddOperand)->getOpcode() == Instruction::Mul) {
					// CASE 1
					MulInst1 = cast<BinaryOperator>(NonAddOperand);
					MulInst0 = GetBinaryOperatorInOperandsInBinaryOperator(NonLastAddInst, Instruction::Mul, Bias);
					if (MulInst0 == nullptr || CheckOtherUseExist(MulInst0, NonLastAddInst)) continue;
					if (CheckOtherUseExist(MulInst1, LastAddInst)) continue;
				} else {
					// CASE 2
					Bias = NonAddOperand;
					Value *MulInst1Value = nullptr;
					MulInst0 = GetBinaryOperatorInOperandsInBinaryOperator(NonLastAddInst, Instruction::Mul, MulInst1Value);
					if (MulInst0 == nullptr || CheckOtherUseExist(MulInst0, NonLastAddInst)) continue;
					if (isa<BinaryOperator>(MulInst1Value) && cast<BinaryOperator>(MulInst1Value)->getOpcode() == Instruction::Mul)
						MulInst1 = cast<BinaryOperator>(MulInst1Value);
					if (CheckOtherUseExist(MulInst1, NonLastAddInst)) continue;
				}
				// Push to Replace List
				MulAddDoublePatternVec.push_back({MulInst0, MulInst1, Bias, LastAddInst, NonLastAddInst});
			}
		}
	}

	Type *VoidTy = Type::getVoidTy(F.getContext());
	Type *Int32Ty = Type::getInt32Ty(F.getContext());
	FunctionType *MoveInstFT = FunctionType::get(VoidTy, {Int32Ty}, false);
	InlineAsm *Move24InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r24, $0", "r", true);
	InlineAsm *Move25InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r25, $0", "r", true);
	InlineAsm *Move26InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r26, $0", "r", true);
	InlineAsm *Move27InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r27, $0", "r", true);
	InlineAsm *Move28InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r28, $0", "r", true);
	FunctionType *CCAInstFT = FunctionType::get(VoidTy, false);
	InlineAsm *CCAInstIA = InlineAsm::get(CCAInstFT, "#removethiscomment cca 2", "", true);
	FunctionType *Move30InstFT = FunctionType::get(Int32Ty, false);
	InlineAsm *Move30InstIA = InlineAsm::get(Move30InstFT, "#removethiscomment move $0, r30", "=r", true);

	for (auto PatternIter = MulAddDoublePatternVec.begin(); PatternIter != MulAddDoublePatternVec.end(); ++PatternIter) {
		BinaryOperator *&MulInst0 = PatternIter->MulInst0;
		BinaryOperator *&MulInst1 = PatternIter->MulInst1;
		BinaryOperator *&LastAddInst = PatternIter->LastAddInst;
		BinaryOperator *&NonLastAddInst = PatternIter->NonLastAddInst;
		Value *&Bias = PatternIter->Bias;
		// CCA Prepare (Implemented as Move instruction)
		CallInst *Move24CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move24InstIA), {MulInst0->getOperand(0)});
		CallInst *Move25CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move25InstIA), {MulInst0->getOperand(1)});
		CallInst *Move26CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move26InstIA), {MulInst1->getOperand(0)});
		CallInst *Move27CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move27InstIA), {MulInst1->getOperand(1)});
		CallInst *Move28CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move28InstIA), {Bias});
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
		Move30CallInst->setTailCall(true);
		// Verbose
		outs() << "[CCA:MulAddDouble] Found Pattern in Function \"" << F.getName() << "\"\n";
		PRINT_INSTRUCTION(" - source.mul0: ", MulInst0);
		PRINT_INSTRUCTION(" - source.mul1: ", MulInst1);
		PRINT_INSTRUCTION(" - source.add0: ", NonLastAddInst);
		PRINT_INSTRUCTION(" - source.add1: ", LastAddInst);
		PRINT_INSTRUCTION(" - output.move24: ", Move24CallInst);
		PRINT_INSTRUCTION(" - output.move25: ", Move25CallInst);
		PRINT_INSTRUCTION(" - output.move26: ", Move26CallInst);
		PRINT_INSTRUCTION(" - output.move27: ", Move27CallInst);
		PRINT_INSTRUCTION(" - output.move28: ", Move28CallInst);
		PRINT_INSTRUCTION(" - output.cca: ", CCACallInst);
		PRINT_INSTRUCTION(" - output.move30: ", Move30CallInst);
		// Insert Instructions
		Move30CallInst->insertAfter(LastAddInst);
		CCACallInst->insertBefore(Move30CallInst);
		Move28CallInst->insertBefore(CCACallInst);
		Move27CallInst->insertBefore(Move28CallInst);
		Move26CallInst->insertBefore(Move27CallInst);
		Move25CallInst->insertBefore(Move26CallInst);
		Move24CallInst->insertBefore(Move25CallInst);
		// Replace & Erase Instructions
		LastAddInst->replaceAllUsesWith(Move30CallInst);
		LastAddInst->eraseFromParent();
		NonLastAddInst->eraseFromParent();
		MulInst0->eraseFromParent();
		MulInst1->eraseFromParent();
	}
	for (auto S : SVecToRemove) { S->eraseFromParent(); }

	return PreservedAnalyses::all();
}

} // namespace cca
} // namespace llvm
