#ifndef PIMCCALLVMPASS_INSTRUMENTATION_CCA_UNIVERSL_PASS_HPP_
#define PIMCCALLVMPASS_INSTRUMENTATION_CCA_UNIVERSL_PASS_HPP_

#include "Instrumentation/CCAPatternGraph.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"

namespace llvm {
namespace cca {

//-------------------------------------
// Class: CCA Pattern Instances
//-------------------------------------
class CCAPattern final {
  private:
	Instruction *O_;
	std::map<unsigned int, Value *> InputRegValueMap_;

	CCAPattern(Instruction *OutputInst) : O_(OutputInst), InputRegValueMap_() {}

  public:
	~CCAPattern() {}
	void print(unsigned int indent, std::ostream &os) const;
	static CCAPattern *get(CCAPatternGraph *Graph,
						   Instruction *StartPoint,
						   std::set<Instruction *> &Removed,
						   const std::set<Instruction *> &Replaced);
	void build(unsigned int ccaid, unsigned int oregnum, char oregtype, LLVMContext &Context);
};

//-------------------------------------
// Class: CCA Universal Pass
//-------------------------------------
class CCAUniversalPass : public PassInfoMixin<CCAUniversalPass> {
  private:
	unsigned int ccaid_;
	char oregtype_;
	unsigned int oregnum_;
	CCAPatternGraph *G_;

  public:
	CCAUniversalPass(std::string patternStr);
	PreservedAnalyses run(Function &, FunctionAnalysisManager &);
	static bool isRequired(void) { return true; }
};

} // namespace cca
} // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_CCA_UNIVERSL_PASS_HPP_
