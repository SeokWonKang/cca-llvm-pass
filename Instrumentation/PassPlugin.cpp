#include "llvm/Passes/PassPlugin.h"
#include "Instrumentation/CCAUniversal.hpp"
#include "Instrumentation/Fixed/CCAFixedPasses.hpp"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

// Pass Registration
PassPluginLibraryInfo getPassPluginInfo() {
	const auto callback = [](PassBuilder &PB) {
		PB.registerOptimizerLastEPCallback([&](ModulePassManager &MPM, auto) {
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("7: o24 = i24 + i25 + i26 + i27 + i28")));
			// MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("8: o24 = i24 + i28; o25 = i25 + o24; o26 = i26 + o25; o27 = i27 +
			// o26")));
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("9: t24 = i24 > i25 ? i24 : i25; o24 = t24 > i26 ? t24 : i26")));
			return true;
		});
	};

	return {LLVM_PLUGIN_API_VERSION, "cca-passes", "0.0.1", callback};
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() { return getPassPluginInfo(); }
