#ifndef PIMCCALLVMPASS_INSTRUMENTATION_CCA_PASSES_HPP_
#define PIMCCALLVMPASS_INSTRUMENTATION_CCA_PASSES_HPP_

#include "llvm/IR/PassManager.h"

namespace llvm {
namespace cca {
// MulAdd
struct CCAMulAddPass : public PassInfoMixin<CCAMulAddPass> {
	PreservedAnalyses run(Function &, FunctionAnalysisManager &);
	static bool isRequired(void) { return true; }
};
// MulAdd Double
struct CCAMulAddDoublePass : public PassInfoMixin<CCAMulAddDoublePass> {
	PreservedAnalyses run(Function &, FunctionAnalysisManager &);
	static bool isRequired(void) { return true; }
};
/*
// MulSubMulDiv
struct CCAMulSubMulDivPass : public PassInfoMixin<CCAMulSubMulDivPass> {
	PreservedAnalyses run(Function &, FunctionAnalysisManager &);
	static bool isRequired(void) { return true; }
};
*/
} // namespace cca
}; // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_CCA_PASSES_HPP_
