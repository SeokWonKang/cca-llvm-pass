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
	std::map<unsigned int, Value *> InputRegValueMap_;
	std::map<unsigned int, Value *> OutputRegValueMap_;
	std::vector<Instruction *> CCAOutputInst_;

	CCAPattern(const std::vector<Instruction *> Candidate) : InputRegValueMap_(), OutputRegValueMap_(), CCAOutputInst_() {}

  public:
	~CCAPattern() {}
	void print(unsigned int indent, std::ostream &os) const;
	static CCAPattern *get(CCAPatternGraph *Graph,
						   const std::vector<Instruction *> &Candidate,
						   std::set<Instruction *> &Removed,
						   const std::set<Instruction *> &UnRemovable);
	void build(unsigned int ccaid, LLVMContext &Context);
	void resolve(void);
	const std::map<unsigned int, Value *> &ORVM(void) const { return OutputRegValueMap_; }
};

//-------------------------------------
// Class: CCA Universal Pass
//-------------------------------------
class CCAUniversalPass : public PassInfoMixin<CCAUniversalPass> {
  private:
	const std::string patternStr_;
	CCAPatternGraph *G_;

  public:
	CCAUniversalPass(std::string patternStr);
	PreservedAnalyses run(Function &, FunctionAnalysisManager &);
	static bool isRequired(void) { return true; }
};

} // namespace cca
} // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_CCA_UNIVERSL_PASS_HPP_
