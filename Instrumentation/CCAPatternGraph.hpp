#ifndef PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_
#define PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_

#include "llvm/IR/Value.h"
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace llvm {
namespace cca {

//-------------------------------------
// Class: CCA Pattern Graph
//-------------------------------------
class CCAPatternGraphNode {
  protected:
	CCAPatternGraphNode *left_, *right_;

  public:
	CCAPatternGraphNode() : left_(nullptr), right_(nullptr) {}
	CCAPatternGraphNode(CCAPatternGraphNode *left, CCAPatternGraphNode *right) : left_(left), right_(right) {}
	virtual ~CCAPatternGraphNode() {
		if (left_ != nullptr) delete left_;
		if (right_ != nullptr) delete right_;
	}
	virtual void print(unsigned int indent, std::ostream &os) const = 0;
	virtual bool matchWithCode(Value *StartPoint,
							   std::set<Instruction *> &Removed,
							   const std::set<Instruction *> &Replaced,
							   std::map<unsigned int, Value *> &InputRegValueMap) const = 0;
};

class CCAPatternGraphOperatorNode final : public CCAPatternGraphNode {
  private:
	char opchar_;

  public:
	CCAPatternGraphOperatorNode(char opchar, CCAPatternGraphNode *left, CCAPatternGraphNode *right)
		: CCAPatternGraphNode(left, right), opchar_(opchar) {}
	virtual ~CCAPatternGraphOperatorNode() {}
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual bool matchWithCode(Value *StartPoint,
							   std::set<Instruction *> &Removed,
							   const std::set<Instruction *> &Replaced,
							   std::map<unsigned, Value *> &InputRegValueMap) const;
};

class CCAPatternGraphRegisterNode final : public CCAPatternGraphNode {
  private:
	char regtype_;
	unsigned int regnum_;

  public:
	CCAPatternGraphRegisterNode(std::string regstr)
		: CCAPatternGraphNode(), regtype_(regstr.at(0)), regnum_(std::atoi(regstr.substr(1, std::string::npos).c_str())) {}
	virtual ~CCAPatternGraphRegisterNode() {}
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual bool matchWithCode(Value *StartPoint,
							   std::set<Instruction *> &Removed,
							   const std::set<Instruction *> &Replaced,
							   std::map<unsigned, Value *> &InputRegValueMap) const;
};

typedef CCAPatternGraphNode CCAPatternGraph;

// Build Graph from Pattern String
CCAPatternGraph *BuildGraph(const std::vector<std::string> &patternTokVec);

} // namespace cca
} // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_
