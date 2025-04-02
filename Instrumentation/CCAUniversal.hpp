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
	static CCAPattern *get(CCAPatternGraph2 *Graph, const std::vector<Instruction *> &Candidate, std::set<Instruction *> &WouldBeRemoved);
	void build(unsigned int ccaid, unsigned int oregnum, char oregtype, LLVMContext &Context);
};

class CCAPattern2 final {
  private:
	const std::vector<Instruction *> C_;
	std::map<unsigned int, Value *> InputRegValueMap_;

	CCAPattern2(const std::vector<Instruction *> Candidate) : C_(Candidate), InputRegValueMap_() {}

  public:
	~CCAPattern2() {}
	void print(unsigned int indent, std::ostream &os) const;
	static CCAPattern2 *get(CCAPatternGraph2 *Graph,
							const std::vector<Instruction *> &Candidate,
							std::set<Instruction *> &Removed,
							const std::set<Instruction *> &UnRemovable);
	void build(unsigned int ccaid, const std::vector<std::pair<unsigned int, char>> &oreginfo, LLVMContext &Context);
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

class CCAUniversalPass2 : public PassInfoMixin<CCAUniversalPass> {
  private:
	unsigned int ccaid_;
	std::vector<std::pair<unsigned int, char>> oreginfo_;
	CCAPatternGraph2 *G_;

  public:
	CCAUniversalPass2(std::string patternStr);
	PreservedAnalyses run(Function &, FunctionAnalysisManager &);
	static bool isRequired(void) { return true; }
};

} // namespace cca
} // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_CCA_UNIVERSL_PASS_HPP_
