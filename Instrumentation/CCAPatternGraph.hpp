#ifndef PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_
#define PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace llvm {
namespace cca {

class CCAPatternGraphNode;
class CCAPatternSubGraph;
class CCAPatternGraphOperatorNode;
class CCAPatternGraphRegisterNode;
class CCAPatternGraphCompareNode;
class CCAPatternGraphSelectNode;
class CCAPatternGraph;

//-------------------------------------------
// Abstract Class: CCA Pattern Graph Node
//-------------------------------------------
class CCAPatternGraphNode {
  public:
	CCAPatternGraphNode() {}
	virtual ~CCAPatternGraphNode() {}
	virtual void print(unsigned int indent, std::ostream &os) const = 0;
	virtual void print(unsigned int indent, llvm::raw_ostream &os) const = 0;
	virtual unsigned opcode(void) const = 0;
	virtual void readyForSearch(void) = 0;
	virtual bool checkValid(void) = 0;
	virtual bool linkSubgraph(char regtype, unsigned regnum, CCAPatternSubGraph *SG) = 0;
	virtual unsigned flagsize(void) = 0;
	virtual void setReverse(const std::vector<bool> &flags, unsigned &index) = 0;
	virtual bool matchWithCode(Value *StartPoint,
							   const std::set<Instruction *> &AlreadyRemoved,
							   std::map<unsigned int, Value *> &IRVM,
							   std::map<unsigned int, Value *> &ORVM,
							   std::map<unsigned int, Value *> &TRVM) const = 0;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) = 0;
	virtual std::string writeInVerilog(std::vector<std::string> &front, std::vector<std::string> &last) = 0;
};

//-------------------------------------------
// Class: CCA Pattern Graph Node
//-------------------------------------------
class CCAPatternSubGraph final : public CCAPatternGraphNode {
  private:
	bool searched_;
	const char regtype_;
	const unsigned int regnum_;
	CCAPatternGraphNode *expr_;

  public:
	CCAPatternSubGraph(char regtype, unsigned regnum, CCAPatternGraphNode *expr)
		: CCAPatternGraphNode(), searched_(false), regtype_(regtype), regnum_(regnum), expr_(expr) {}
	CCAPatternSubGraph(std::string regstr, CCAPatternGraphNode *expr)
		: CCAPatternGraphNode(), regtype_(regstr.at(0)), regnum_(std::atoi(regstr.substr(1, std::string::npos).c_str())), expr_(expr) {}
	virtual ~CCAPatternSubGraph() {
		if (expr_ != nullptr) delete expr_;
		expr_ = nullptr;
	}
	virtual void print(unsigned indent, std::ostream &os) const;
	virtual void print(unsigned indent, llvm::raw_ostream &os) const;

	char regtype(void) const { return regtype_; }
	unsigned regnum(void) const { return regnum_; }
	virtual unsigned opcode(void) const {
		if (expr_ != nullptr) return expr_->opcode();
		return Instruction::OtherOpsEnd;
	}
	virtual void readyForSearch(void) {
		searched_ = false;
		if (expr_ != nullptr) expr_->readyForSearch();
	}
	virtual bool checkValid(void) {
		if (searched_) return true;
		searched_ = true;
		if (expr_ != nullptr) return expr_->checkValid();
		return false;
	}
	virtual bool linkSubgraph(char regtype, unsigned int regnum, CCAPatternSubGraph *SG) {
		if (searched_) return false;
		searched_ = true;
		if (expr_ != nullptr) return expr_->linkSubgraph(regtype, regnum, SG);
		return false;
	}
	virtual unsigned flagsize(void) {
		if (searched_) return 0;
		searched_ = true;
		if (expr_ != nullptr) return expr_->flagsize();
		return 0;
	}
	virtual void setReverse(const std::vector<bool> &flags, unsigned &index) {
		if (searched_) return;
		searched_ = true;
		if (expr_ != nullptr) expr_->setReverse(flags, index);
	}
	virtual bool matchWithCode(Value *StartPoint,
							   const std::set<Instruction *> &AlreadyRemoved,
							   std::map<unsigned int, Value *> &IRVM,
							   std::map<unsigned int, Value *> &ORVM,
							   std::map<unsigned int, Value *> &TRVM) const {
		if (expr_ != nullptr) return expr_->matchWithCode(StartPoint, AlreadyRemoved, IRVM, ORVM, TRVM);
		return false;
	}
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) {
		if (searched_) return;
		searched_ = true;
		if (expr_ != nullptr) expr_->getRemoveList(StartPoint, nullptr, RemoveList);
	}
	virtual std::string writeInVerilog(std::vector<std::string> &front, std::vector<std::string> &last) {
		std::string regstr = std::string(1, regtype_) + std::to_string(regnum_);
		if (searched_) return regstr;
		searched_ = true;
		front.push_back("reg [0:64] " + regstr + "\n");
		front.push_back(regstr + " <= " + expr_->writeInVerilog(front, last) + "\n");
		if (regtype_ == 'o') last.push_back("assign r" + std::to_string(regnum_) + "_out = " + regstr + "\n");
		return regstr;
	}
};

class CCAPatternGraphRegisterNode final : public CCAPatternGraphNode {
  private:
	const char regtype_;
	const unsigned int regnum_;
	CCAPatternSubGraph *SG_;

  public:
	CCAPatternGraphRegisterNode(std::string regstr)
		: CCAPatternGraphNode(), regtype_(regstr.at(0)), regnum_(std::atoi(regstr.substr(1, std::string::npos).c_str())), SG_(nullptr) {}
	virtual ~CCAPatternGraphRegisterNode() { SG_ = nullptr; }
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual void print(unsigned int indent, llvm::raw_ostream &os) const;
	virtual unsigned opcode(void) const { return Instruction::OtherOpsEnd; }

	char regtype(void) const { return regtype_; }
	unsigned regnum(void) const { return regnum_; }

	virtual void readyForSearch(void);
	virtual bool checkValid(void);
	virtual bool linkSubgraph(char regtype, unsigned regnum, CCAPatternSubGraph *SG);
	virtual unsigned flagsize(void);
	virtual void setReverse(const std::vector<bool> &flags, unsigned &index);
	virtual bool matchWithCode(Value *StartPoint,
							   const std::set<Instruction *> &AlreadyRemoved,
							   std::map<unsigned, Value *> &IRVM,
							   std::map<unsigned, Value *> &ORVM,
							   std::map<unsigned, Value *> &TRVM) const;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList);
	virtual std::string writeInVerilog(std::vector<std::string> &front, std::vector<std::string> &last) {
		std::string regstr = std::string(1, regtype_) + std::to_string(regnum_);
		if(regtype_ == 'i') regstr = "r" + std::to_string(regnum_) + "_in";
		if (SG_ == nullptr) return regstr;
		else
			return SG_->writeInVerilog(front, last);
	}
};

class CCAPatternGraphOperatorNode final : public CCAPatternGraphNode {
  private:
	CCAPatternGraphNode *left_;
	CCAPatternGraphNode *right_;
	bool reversed_;
	std::string op_;

  public:
	CCAPatternGraphOperatorNode(std::string op, CCAPatternGraphNode *left, CCAPatternGraphNode *right)
		: CCAPatternGraphNode(), left_(left), right_(right), reversed_(false), op_(op) {}
	virtual ~CCAPatternGraphOperatorNode() {
		if (left_ != nullptr) delete left_;
		left_ = nullptr;
		if (right_ != nullptr) delete right_;
		right_ = nullptr;
	}
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual void print(unsigned int indent, llvm::raw_ostream &os) const;

	std::string opstr(void) const { return op_; }
	bool reversable(void) const { return op_ == "+" || op_ == "*"; }

	virtual unsigned opcode(void) const {
		if (op_ == "+") return Instruction::Add;
		else if (op_ == "-")
			return Instruction::Sub;
		else if (op_ == "*")
			return Instruction::Mul;
		else if (op_ == "/")
			return Instruction::UDiv;
		else
			return Instruction::OtherOpsEnd;
	}
	virtual void readyForSearch(void);
	virtual bool checkValid(void);
	virtual bool linkSubgraph(char regtype, unsigned regnum, CCAPatternSubGraph *SG);
	virtual unsigned flagsize(void);
	virtual void setReverse(const std::vector<bool> &flags, unsigned &index);
	virtual bool matchWithCode(Value *StartPoint,
							   const std::set<Instruction *> &AlreadyRemoved,
							   std::map<unsigned, Value *> &IRVM,
							   std::map<unsigned, Value *> &ORVM,
							   std::map<unsigned, Value *> &TRVM) const;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList);
	virtual std::string writeInVerilog(std::vector<std::string> &front, std::vector<std::string> &last) {
		return "(" + left_->writeInVerilog(front, last) + op_ + right_->writeInVerilog(front, last) + ")";
	}
};

class CCAPatternGraphCompareNode final : public CCAPatternGraphNode {
  private:
	CCAPatternGraphNode *left_;
	CCAPatternGraphNode *right_;
	bool reversed_;
	std::string op_;

  public:
	CCAPatternGraphCompareNode(std::string op, CCAPatternGraphNode *left, CCAPatternGraphNode *right)
		: CCAPatternGraphNode(), left_(left), right_(right), reversed_(false), op_(op) {}
	virtual ~CCAPatternGraphCompareNode() {
		if (left_ != nullptr) delete left_;
		left_ = nullptr;
		if (right_ != nullptr) delete right_;
		right_ = nullptr;
	}
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual void print(unsigned int indent, llvm::raw_ostream &os) const;
	virtual unsigned opcode(void) const { return Instruction::ICmp; }

	std::string opstr(void) const { return op_; }
	CmpInst::Predicate predicate(void) const {
		if (op_ == "==") return CmpInst::Predicate::ICMP_EQ;
		else if (op_ == "!=")
			return CmpInst::Predicate::ICMP_NE;
		else if (op_ == "<")
			return CmpInst::Predicate::ICMP_SLT;
		else if (op_ == "<=")
			return CmpInst::Predicate::ICMP_SLE;
		else if (op_ == ">")
			return CmpInst::Predicate::ICMP_SGT;
		else if (op_ == ">=")
			return CmpInst::Predicate::ICMP_SGE;
		return CmpInst::Predicate::BAD_ICMP_PREDICATE;
	}
	bool reversable(void) const { return op_ == "==" || op_ == "!="; }

	virtual void readyForSearch(void);
	virtual bool checkValid(void);
	virtual bool linkSubgraph(char regtype, unsigned int regnum, CCAPatternSubGraph *SG);
	virtual unsigned flagsize(void);
	virtual void setReverse(const std::vector<bool> &flags, unsigned &index);
	virtual bool matchWithCode(Value *StartPoint,
							   const std::set<Instruction *> &AlreadyRemoved,
							   std::map<unsigned int, Value *> &IRVM,
							   std::map<unsigned int, Value *> &ORVM,
							   std::map<unsigned int, Value *> &TRVM) const;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList);
	virtual std::string writeInVerilog(std::vector<std::string> &front, std::vector<std::string> &last) {
		return "(" + left_->writeInVerilog(front, last) + op_ + right_->writeInVerilog(front, last) + ")";
	}
};

class CCAPatternGraphSelectNode final : public CCAPatternGraphNode {
  private:
	CCAPatternGraphCompareNode *cmp_;
	CCAPatternGraphNode *true_expr_;
	CCAPatternGraphNode *false_expr_;

  public:
	CCAPatternGraphSelectNode(CCAPatternGraphCompareNode *cmp, CCAPatternGraphNode *true_expr, CCAPatternGraphNode *false_expr)
		: CCAPatternGraphNode(), cmp_(cmp), true_expr_(true_expr), false_expr_(false_expr) {}
	virtual ~CCAPatternGraphSelectNode() {
		if (cmp_ != nullptr) delete cmp_;
		cmp_ = nullptr;
		if (true_expr_ != nullptr) delete true_expr_;
		true_expr_ = nullptr;
		if (false_expr_ != nullptr) delete false_expr_;
		false_expr_ = nullptr;
	}
	virtual void print(unsigned int indent, std::ostream &os) const;
	virtual void print(unsigned int indent, llvm::raw_ostream &os) const;
	virtual unsigned opcode(void) const { return Instruction::Select; }

	virtual void readyForSearch(void);
	virtual bool checkValid(void);
	virtual bool linkSubgraph(char regtype, unsigned int regnum, CCAPatternSubGraph *SG);
	virtual unsigned flagsize(void);
	virtual void setReverse(const std::vector<bool> &flags, unsigned &index);
	virtual bool matchWithCode(Value *StartPoint,
							   const std::set<Instruction *> &AlreadyRemoved,
							   std::map<unsigned int, Value *> &IRVM,
							   std::map<unsigned int, Value *> &ORVM,
							   std::map<unsigned int, Value *> &TRVM) const;
	virtual void getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList);

	virtual std::string writeInVerilog(std::vector<std::string> &front, std::vector<std::string> &last) {
		return "(" + cmp_->writeInVerilog(front, last) + " ? " + true_expr_->writeInVerilog(front, last) + " : " +
			   false_expr_->writeInVerilog(front, last) + ")";
	}
};

//-------------------------------------------
// Class: CCA Pattern Graph
//-------------------------------------------

class CCAPatternGraph final {
  private:
	const unsigned rule_number_;
	std::vector<CCAPatternSubGraph *> graphs_;
	std::vector<CCAPatternSubGraph *> linked_graphs_;

  public:
	CCAPatternGraph(unsigned rule_number, const std::vector<CCAPatternSubGraph *> SubGraphs);
	~CCAPatternGraph() {
		for (auto &G : graphs_) {
			delete G;
			G = nullptr;
		}
		graphs_.clear();
		linked_graphs_.clear();
	}
	std::vector<unsigned> opcode(void) const {
		std::vector<unsigned> retval;
		for (auto &SG : linked_graphs_) retval.push_back(SG->opcode());
		return retval;
	}
	std::vector<std::pair<char, unsigned int>> output_reginfo(void) const {
		std::vector<std::pair<char, unsigned int>> retval;
		for (const auto &SG : linked_graphs_) retval.push_back(std::pair<char, unsigned int>(SG->regtype(), SG->regnum()));
		return retval;
	}
	unsigned rule_number(void) const { return rule_number_; }
	void print(unsigned int indent, std::ostream &os) const;
	void print(unsigned int indent, llvm::raw_ostream &os) const;
	bool matchWithCode(const std::vector<Instruction *> &Candidate,
					   const std::set<Instruction *> &UnRemovable,
					   std::set<Instruction *> &Removed,
					   std::map<unsigned, Value *> &InputRegValueMap,
					   std::map<unsigned, Value *> &OutputRegValueMap) const;
	std::string writeInVerilog(void) const;
};

} // namespace cca
} // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_CCA_PATTERN_GRAPH_HPP_
