#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct InsertCommentedInlineAsmBeforeRetInst : public PassInfoMixin<InsertCommentedInlineAsmBeforeRetInst> {
	// Run
	PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
		for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
			// Get Terminator Instruction and Check Return Instruction
			Instruction *TerminatorInst = FuncIter->getTerminator();
			if (!isa<ReturnInst>(TerminatorInst)) continue;
			ReturnInst *RI = cast<ReturnInst>(TerminatorInst);

			Type *Int32Ty = Type::getInt32Ty(F.getContext());
			if (RI->getReturnValue() != nullptr && RI->getReturnValue()->getType() == Int32Ty) {
				FunctionType *IAFT = FunctionType::get(Int32Ty, {Int32Ty}, false);
				InlineAsm *IA = InlineAsm::get(IAFT, "#removethiscomment ICIABRI $0", "=r,r", true);
				CallInst *CI = CallInst::Create(FunctionCallee(IAFT, IA), {RI->getReturnValue()}, "inlineasmtestinst", TerminatorInst);
				CI->setTailCall(true);
			} else {
				// Insert Commented Inline Assembly Before Return Instruction
				FunctionType *IAFT = FunctionType::get(Int32Ty, false);
				InlineAsm *IA = InlineAsm::get(IAFT, "#removethiscomment ICIABRI", "=r", true);
				CallInst *CI = CallInst::Create(FunctionCallee(IAFT, IA), "inlineasmtestinst", TerminatorInst);
				CI->setTailCall(true);
			}

			outs() << "ICIABRI: Function \"" << F.getName() << "\"\n";
			// CI->print(outs());
			// outs() << '\n';
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
			MPM.addPass(createModuleToFunctionPassAdaptor(InsertCommentedInlineAsmBeforeRetInst()));
			return true;
		});
	};

	return {LLVM_PLUGIN_API_VERSION, "insert-commented-inlineasm-before-return-inst", "0.0.1", callback};
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() { return getPassPluginInfo(); }
