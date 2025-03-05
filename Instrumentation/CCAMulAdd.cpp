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
struct MulAddPattern {
	BinaryOperator *AddInst;
	BinaryOperator *MulInst;
	Value *AddOperand;
};

// Run
PreservedAnalyses CCAMulAddPass::run(Function &F, FunctionAnalysisManager &) {
	std::vector<struct MulAddPattern> MulAddPatternVec;

	// Find MulAdd Patterns
	for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
		for (BasicBlock::iterator BBIter = FuncIter->begin(); BBIter != FuncIter->end(); ++BBIter) {
			if (isa<BinaryOperator>(BBIter) && cast<BinaryOperator>(BBIter)->getOpcode() == Instruction::Add) {
				// Find Mul-Add Patterns
				BinaryOperator *AddInst = cast<BinaryOperator>(BBIter);
				BinaryOperator *MulInst = nullptr;
				Value *AddOperand = nullptr;
				MulInst = GetBinaryOperatorInOperandsInBinaryOperator(AddInst, Instruction::Mul, AddOperand);
				if (MulInst == nullptr) continue;
				if (CheckOtherUseExist(MulInst, AddInst)) continue;

				// Check All the Operands be from Load or PHINode
				/*
				if (!(isa<LoadInst>(MulInst->getOperand(0)) || isa<PHINode>(MulInst->getOperand(0))) ||
					!(isa<LoadInst>(MulInst->getOperand(1)) || isa<PHINode>(MulInst->getOperand(1))) ||
					!(isa<LoadInst>(AddOperand) || isa<PHINode>(AddOperand))) {
					outs() << "[CCA:MulAdd] Found MulAdd Pattern but Operands are not Load or PHINode in Function \"" << F.getName() << "\"\n";
					continue;
				}
				*/

				// outs() << "[CCA:MulAdd] Found Mul-Add Pattern in Function \"" << F.getName() << "\"\n";
				// outs() << "  - mul: "; MulInst->print(outs()); outs() << '\n';
				// outs() << "  - add: "; AddInst->print(outs()); outs() << '\n';

				MulAddPatternVec.push_back({AddInst, MulInst, AddOperand});
			}
		}
	}

	Type *VoidTy = Type::getVoidTy(F.getContext());
	Type *Int32Ty = Type::getInt32Ty(F.getContext());
	FunctionType *MoveInstFT = FunctionType::get(VoidTy, {Int32Ty}, false);
	InlineAsm *Move24InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r24, $0", "r", true);
	InlineAsm *Move25InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r25, $0", "r", true);
	InlineAsm *Move26InstIA = InlineAsm::get(MoveInstFT, "#removethiscomment move r26, $0", "r", true);
	FunctionType *CCAInstFT = FunctionType::get(VoidTy, false);
	InlineAsm *CCAInstIA = InlineAsm::get(CCAInstFT, "#removethiscomment cca 0", "", true);
	FunctionType *Move30InstFT = FunctionType::get(Int32Ty, false);
	InlineAsm *Move30InstIA = InlineAsm::get(Move30InstFT, "#removethiscomment move $0, r30", "=r", true);

	for (auto PatternIter = MulAddPatternVec.begin(); PatternIter != MulAddPatternVec.end(); ++PatternIter) {
		BinaryOperator *AddInst = PatternIter->AddInst;
		BinaryOperator *MulInst = PatternIter->MulInst;
		Value *AddOperand = PatternIter->AddOperand;
		// CCA Prepare (Implemented as Move instruction)
		CallInst *Move24CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move24InstIA), {MulInst->getOperand(0)});
		CallInst *Move25CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move25InstIA), {MulInst->getOperand(1)});
		CallInst *Move26CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move26InstIA), {AddOperand});
		Move24CallInst->setTailCall(true);
		Move25CallInst->setTailCall(true);
		Move26CallInst->setTailCall(true);
		// // CCA Instruction
		CallInst *CCACallInst = CallInst::Create(FunctionCallee(CCAInstFT, CCAInstIA));
		CCACallInst->setTailCall(true);
		// Store Instruction
		CallInst *Move30CallInst = CallInst::Create(FunctionCallee(Move30InstFT, Move30InstIA), {}, "move30inst");
		Move30CallInst->setTailCall(true);
		// Verbose
		outs() << "[CCA:MulAdd] Found MulAdd Pattern in Function \"" << F.getName() << "\"\n";
		PRINT_INSTRUCTION(" - source.mul: ", MulInst);
		PRINT_INSTRUCTION(" - source.add: ", AddInst);
		PRINT_INSTRUCTION(" - output.move24: ", Move24CallInst);
		PRINT_INSTRUCTION(" - output.move25: ", Move25CallInst);
		PRINT_INSTRUCTION(" - output.move26: ", Move26CallInst);
		PRINT_INSTRUCTION(" - output.cca: ", CCACallInst);
		PRINT_INSTRUCTION(" - output.move30: ", Move30CallInst);
		// Insert Instructions
		Move30CallInst->insertAfter(AddInst);
		CCACallInst->insertBefore(Move30CallInst);
		Move26CallInst->insertBefore(CCACallInst);
		Move25CallInst->insertBefore(Move26CallInst);
		Move24CallInst->insertBefore(Move25CallInst);
		// Replace & Erase Instructions
		AddInst->replaceAllUsesWith(Move30CallInst);
		AddInst->eraseFromParent();
		MulInst->eraseFromParent();
	}

	return PreservedAnalyses::all();
}

} // namespace cca
} // namespace llvm
