#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

using namespace llvm;

namespace {

#define PRINT_INSTRUCTION(Header, Inst)                                                                                                              \
	outs() << Header;                                                                                                                                \
	(Inst)->print(outs());                                                                                                                           \
	outs() << '\n';

struct MulAddPattern {
	BinaryOperator *AddInst;
	BinaryOperator *MulInst;
	Value *AddOperand;
};

struct CCAMulAddPass : public PassInfoMixin<CCAMulAddPass> {
	// Run
	PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
		std::vector<struct MulAddPattern> MulAddPatternVec;

		if (F.getName() != "gemv") return PreservedAnalyses::all();

		// Find MulAdd Patterns
		for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
			for (BasicBlock::iterator BBIter = FuncIter->begin(); BBIter != FuncIter->end(); ++BBIter) {
				if (isa<BinaryOperator>(BBIter) && cast<BinaryOperator>(BBIter)->getOpcode() == Instruction::Add) {
					BinaryOperator *AddInst = cast<BinaryOperator>(BBIter);
					BinaryOperator *MulInst = nullptr;
					Value *AddOperand = nullptr;
					if (isa<BinaryOperator>(AddInst->getOperand(0)) &&
						cast<BinaryOperator>(AddInst->getOperand(0))->getOpcode() == Instruction::Mul) {
						MulInst = cast<BinaryOperator>(AddInst->getOperand(0));
						AddOperand = AddInst->getOperand(1);
					} else if (isa<BinaryOperator>(AddInst->getOperand(1)) &&
							   cast<BinaryOperator>(AddInst->getOperand(1))->getOpcode() == Instruction::Mul) {
						MulInst = cast<BinaryOperator>(AddInst->getOperand(1));
						AddOperand = AddInst->getOperand(0);
					}
					if (MulInst == nullptr) continue;
					// outs() << "[CCA:MulAdd] Found Mul-Add Pattern in Function \"" << F.getName() << "\"\n";
					// outs() << "  - mul: "; MulInst->print(outs()); outs() << '\n';
					// outs() << "  - add: "; AddInst->print(outs()); outs() << '\n';

					bool OtherUseExist = false;
					for (auto MulInstUseIter : MulInst->users()) {
						if (MulInstUseIter != AddInst) {
							// outs() << "other use found! : ";
							// MulInstUseIter->print(outs()); outs() << '\n';
							OtherUseExist = true;
							break;
						}
					}
					if (OtherUseExist) continue;
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
		FunctionType *StoreInstFT = FunctionType::get(Int32Ty, false);
		InlineAsm *Store30IA = InlineAsm::get(StoreInstFT, "#removethiscomment sw $0, 0, r30", "=r", true);

		for (auto PatternIter = MulAddPatternVec.begin(); PatternIter != MulAddPatternVec.end(); ++PatternIter) {
			BinaryOperator *AddInst = PatternIter->AddInst;
			BinaryOperator *MulInst = PatternIter->MulInst;
			Value *AddOperand = PatternIter->AddOperand;
			// CCA Prepare (Implemented as Move instruction)
			CallInst *Move24CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move24InstIA), {AddOperand});
			CallInst *Move25CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move25InstIA), {MulInst->getOperand(0)});
			CallInst *Move26CallInst = CallInst::Create(FunctionCallee(MoveInstFT, Move26InstIA), {MulInst->getOperand(1)});
			Move24CallInst->setTailCall(true);
			Move25CallInst->setTailCall(true);
			Move26CallInst->setTailCall(true);
			// // CCA Instruction
			CallInst *CCACallInst = CallInst::Create(FunctionCallee(CCAInstFT, CCAInstIA));
			CCACallInst->setTailCall(true);
			// Store Instruction
			CallInst *StoreCallInst = CallInst::Create(FunctionCallee(StoreInstFT, Store30IA), {}, "store30inst");
			StoreCallInst->setTailCall(true);
			// Verbose
			outs() << "[CCA:MulAdd] Found MulAdd Pattern in Function \"" << F.getName() << "\"\n";
			PRINT_INSTRUCTION(" - source.mul: ", MulInst);
			PRINT_INSTRUCTION(" - source.add: ", AddInst);
			PRINT_INSTRUCTION(" - output.move24: ", Move24CallInst);
			PRINT_INSTRUCTION(" - output.move25: ", Move25CallInst);
			PRINT_INSTRUCTION(" - output.move26: ", Move26CallInst);
			PRINT_INSTRUCTION(" - output.cca: ", CCACallInst);
			PRINT_INSTRUCTION(" - output.store30: ", StoreCallInst);
			// Insert Instructions
			StoreCallInst->insertAfter(AddInst);
			CCACallInst->insertBefore(StoreCallInst);
			Move26CallInst->insertBefore(CCACallInst);
			Move25CallInst->insertBefore(Move26CallInst);
			Move24CallInst->insertBefore(Move25CallInst);
			// Replace & Erase Instructions
			AddInst->replaceAllUsesWith(StoreCallInst);
			AddInst->eraseFromParent();
			MulInst->eraseFromParent();
		}

		return PreservedAnalyses::all();
	}
	// Required (for not skipping)
	static bool isRequired() { return true; }
};
} // namespace

// Pass Registration
PassPluginLibraryInfo getPassPluginInfo() {
	const auto callback = [](PassBuilder &PB) {
		PB.registerPipelineEarlySimplificationEPCallback([&](ModulePassManager &MPM, auto) {
			MPM.addPass(createModuleToFunctionPassAdaptor(CCAMulAddPass()));
			return true;
		});
	};

	return {LLVM_PLUGIN_API_VERSION, "cca-muladd-pass", "0.0.1", callback};
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() { return getPassPluginInfo(); }
