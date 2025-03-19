#include "Instrumentation/CCAPasses.hpp"
#include "Instrumentation/CCAUniversal.hpp"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

// Pass Registration
PassPluginLibraryInfo getPassPluginInfo() {
	const auto callback = [](PassBuilder &PB) {
		PB.registerOptimizerLastEPCallback([&](ModulePassManager &MPM, auto) {
			// MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAMulSubMulDivPass()));
			// MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAMulAddDoublePass()));
			// MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAMulAddPass()));
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("3: r30 = r24 * r25 - (r26 * r27) / r28")));
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("3: r30 = r24 * r25 - r26 * (r27 / r28)")));
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("2: r30 = (r24 * r25 + r26 * r27) + r28")));
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("2: r30 = r24 * r25 + (r26 * r27 + r28)")));
			MPM.addPass(createModuleToFunctionPassAdaptor(cca::CCAUniversalPass("0: r30 = r24 * r25 + r26")));
			return true;
		});
	};

	return {LLVM_PLUGIN_API_VERSION, "cca-passes", "0.0.1", callback};
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() { return getPassPluginInfo(); }
