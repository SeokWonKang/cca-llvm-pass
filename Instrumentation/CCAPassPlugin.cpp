#include "Instrumentation/CCAPasses.hpp"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

// Pass Registration
PassPluginLibraryInfo getPassPluginInfo() {
	const auto callback = [](PassBuilder &PB) {
		PB.registerOptimizerLastEPCallback([&](ModulePassManager &MPM, auto) {
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAMulAddDoublePass()));
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAMulAddPass()));
			return true;
		});
	};

	return {LLVM_PLUGIN_API_VERSION, "cca-passes", "0.0.1", callback};
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() { return getPassPluginInfo(); }
