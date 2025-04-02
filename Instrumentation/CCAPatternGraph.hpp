#ifndef PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_
#define PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_

#include "llvm/IR/Instruction.h"
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
		left_ = nullptr;
		if (right_ != nullptr) delete right_;
		right_ = nullptr;
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

//-------------------------------------
// Class: CCA Pattern Graph
//-------------------------------------
class CCAPatternGraphNode2 {
  protected:
	CCAPatternGraphNode2 *left_, *right_;

  public:
	CCAPatternGraphNode2() : left_(nullptr), right_(nullptr) {}
	CCAPatternGraphNode2(CCAPatternGraphNode2 *left, CCAPatternGraphNode2 *right) : left_(left), right_(right) {}
	virtual ~CCAPatternGraphNode2() {
		if (left_ != nullptr) delete left_;
		if (right_ != nullptr) delete right_;
	}
	virtual Instruction::BinaryOps getBOS(void) const { return Instruction::BinaryOpsEnd; }
	virtual void print(unsigned int indent, std::ostream &os) const = 0;
	virtual unsigned flagsize(void) const { return 0; }
	virtual void reverse(const std::vector<bool> &flags, unsigned &index) {}
	virtual bool matchWithCode(Value *StartPoint,
							   const std::set<Instruction *> &AlreadyRemoved,
							   std::map<unsigned int, Value *> &InputRegValueMap) const = 0;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) const = 0;
};

class CCAPatternGraphOperatorNode2 final : public CCAPatternGraphNode2 {
  private:
	bool reversed_;
	char opchar_;

  public:
	CCAPatternGraphOperatorNode2(char opchar, CCAPatternGraphNode2 *left, CCAPatternGraphNode2 *right)
		: CCAPatternGraphNode2(left, right), reversed_(false), opchar_(opchar) {}
	virtual ~CCAPatternGraphOperatorNode2() {}
	virtual Instruction::BinaryOps getBOS(void) const {
		switch (opchar_) {
		case '+': return Instruction::Add; break;
		case '-': return Instruction::Sub; break;
		case '*': return Instruction::Mul; break;
		case '/': return Instruction::UDiv; break;
		default: return Instruction::BinaryOpsEnd;
		}
	}
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual unsigned flagsize(void) const;
	virtual void reverse(const std::vector<bool> &flags, unsigned &index);
	virtual bool matchWithCode(Value *StartPoint, const std::set<Instruction *> &AlreadyRemoved, std::map<unsigned, Value *> &InputRegValueMap) const;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) const;
};

class CCAPatternGraphRegisterNode2 final : public CCAPatternGraphNode2 {
  private:
	char regtype_;
	unsigned int regnum_;

  public:
	CCAPatternGraphRegisterNode2(std::string regstr)
		: CCAPatternGraphNode2(), regtype_(regstr.at(0)), regnum_(std::atoi(regstr.substr(1, std::string::npos).c_str())) {}
	virtual ~CCAPatternGraphRegisterNode2() {}
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual bool matchWithCode(Value *StartPoint, const std::set<Instruction *> &AlreadyRemoved, std::map<unsigned, Value *> &InputRegValueMap) const;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) const;
};

class CCAPatternGraph2 final {
  private:
	std::vector<CCAPatternGraphNode2 *> graphs_;

  public:
	CCAPatternGraph2() : graphs_() {};
	~CCAPatternGraph2() {
		for (auto &G : graphs_) delete G;
		graphs_.clear();
	}
	void setGraph(const std::vector<CCAPatternGraphNode2 *> &graphs) { graphs_ = graphs; }
	std::vector<Instruction::BinaryOps> getBOS(void) const {
		std::vector<Instruction::BinaryOps> retval;
		for (auto &G : graphs_) retval.push_back(G->getBOS());
		return retval;
	}
	void print(unsigned int indent, unsigned int graphidx, std::ostream &os) const;
	bool matchWithCode(const std::vector<Instruction *> &Candidate,
					   const std::set<Instruction *> &UnRemovable,
					   std::set<Instruction *> &Removed,
					   std::map<unsigned, Value *> &InputRegValueMap) const;
};

// Build Graph from Pattern String
CCAPatternGraph *BuildGraph(const std::vector<std::string> &patternTokVec);
CCAPatternGraphNode2 *BuildGraph2(const std::vector<std::string> &patternTokVec);

} // namespace cca
} // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_
