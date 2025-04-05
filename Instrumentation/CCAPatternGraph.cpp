#include "Instrumentation/CCAPatternGraph.hpp"
#include "Instrumentation/Utils.hpp"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <stack>
#include <string>
#include <vector>

namespace llvm {
namespace cca {

/*
//-------------------------------------------
// Build (Sub)Graph from Pattern String
//-------------------------------------------
inline bool isOpTok(const std::string &tok) {
	if (tok == "+" || tok == "-" || tok == "*" || tok == "/") return true;
	else
		return false;
}

inline int tokPriority(const std::string &tok) {
	if (tok == "*" || tok == "/") return 2;
	else if (tok == "+" || tok == "-")
		return 1;
	return 0;
}
inline std::vector<std::string> ConvertInfixToPostfix(const std::vector<std::string> &InfixTokenVec) {
	std::vector<std::string> PostfixTokenVec;
	std::stack<std::string> st;
	for (auto &tok : InfixTokenVec) {
		if (tok == "(") {
			st.push(tok);
			continue;
		} else if (tok == ")") {
			while (!st.empty() && st.top() != "(") {
				PostfixTokenVec.push_back(st.top());
				st.pop();
			}
			st.pop();
			continue;
		}
		if (!isOpTok(tok)) PostfixTokenVec.push_back(tok);
		else {
			while (!st.empty() && tokPriority(tok) <= tokPriority(st.top())) {
				PostfixTokenVec.push_back(st.top());
				st.pop();
			}
			st.push(tok);
		}
	}
	while (!st.empty()) {
		PostfixTokenVec.push_back(st.top());
		st.pop();
	}
	return PostfixTokenVec;
}
inline std::vector<std::string> ConvertInfixToPrefix(const std::vector<std::string> &InfixTokenVec) {
	std::vector<std::string> TokenVec = InfixTokenVec;
	reverse(TokenVec.begin(), TokenVec.end());
	for (auto &tok : TokenVec) {
		if (tok == "(") tok = ")";
		else if (tok == ")")
			tok = "(";
	}
	std::vector<std::string> PrefixTokenVec = ConvertInfixToPostfix(TokenVec);
	reverse(PrefixTokenVec.begin(), PrefixTokenVec.end());
	return PrefixTokenVec;
}

inline CCAPatternGraphNode *BuildCCAPatternGraph2FromPostfixTokens(const std::vector<std::string> &PostfixTokenVec) {
	std::stack<CCAPatternGraphNode *> st;
	for (auto &tok : PostfixTokenVec) {
		if (isOpTok(tok)) {
			CCAPatternGraphNode *rightNode = st.top();
			st.pop();
			CCAPatternGraphNode *leftNode = st.top();
			st.pop();
			CCAPatternGraphNode *opNode = new CCAPatternGraphOperatorNode(tok[0], leftNode, rightNode);
			st.push(opNode);
		} else
			st.push(new CCAPatternGraphRegisterNode(tok));
	}
	return st.top();
}

// CCA Pattern SubGraph
CCAPatternSubGraph::CCAPatternSubGraph(char regtype, unsigned regnum, const std::vector<std::string> &patternTokenVec)
	: CCAPatternGraphNode(), regtype_(regtype), regnum_(regnum), G_(nullptr) {
	G_ = BuildCCAPatternGraph2FromPostfixTokens(ConvertInfixToPostfix(patternTokenVec));
}
*/

//-------------------------------------------
// Class: CCA Pattern Graph
//-------------------------------------------
// Print Node & Graph
void CCAPatternSubGraph::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "assignment ";
	switch (regtype_) {
	case 'o': os << "output "; break;
	case 't': os << "temporary "; break;
	default: os << "unknown (type " << regtype_ << ") "; break;
	}
	os << '\'' << regnum_ << '\'';
	if (expr_ == nullptr) os << " : unliked\n";
	else {
		os << '\n';
		expr_->print(indent + 2, os);
	}
}

void CCAPatternSubGraph::print(unsigned int indent, llvm::raw_ostream &os) const {
	os << std::string(indent, ' ') << "assignment ";
	switch (regtype_) {
	case 'o': os << "output "; break;
	case 't': os << "temporary "; break;
	default: os << "unknown (type " << regtype_ << ") "; break;
	}
	os << '\'' << regnum_ << '\'';
	if (expr_ == nullptr) os << " : unliked\n";
	else {
		os << '\n';
		expr_->print(indent + 2, os);
	}
}

void CCAPatternGraphRegisterNode::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "register ";
	switch (regtype_) {
	case 'i': os << "input "; break;
	case 'o': os << "output "; break;
	case 't': os << "temporary "; break;
	default: os << "unknown (type " << regtype_ << ") "; break;
	}
	os << '\'' << regnum_ << '\'';
	if (regtype_ == 'o' || regtype_ == 't') {
		if (SG_ == nullptr) os << " : unliked\n";
		else {
			os << '\n';
			SG_->print(indent, os);
		}
	} else
		os << '\n';
}

void CCAPatternGraphRegisterNode::print(unsigned int indent, llvm::raw_ostream &os) const {
	os << std::string(indent, ' ') << "register ";
	switch (regtype_) {
	case 'i': os << "input "; break;
	case 'o': os << "output "; break;
	case 't': os << "temporary "; break;
	default: os << "unknown (type " << regtype_ << ") "; break;
	}
	os << '\'' << regnum_ << '\'';
	if (regtype_ == 'o' || regtype_ == 't') {
		if (SG_ == nullptr) os << " : unliked\n";
		else {
			os << " : linked below\n";
			SG_->print(indent, os);
		}
	} else
		os << '\n';
}

void CCAPatternGraphOperatorNode::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "operator \'" << op_ << '\'' << std::endl;
	left_->print(indent + 2, os);
	right_->print(indent + 2, os);
}

void CCAPatternGraphOperatorNode::print(unsigned int indent, llvm::raw_ostream &os) const {
	os << std::string(indent, ' ') << "operator \'" << op_ << "\'\n";
	left_->print(indent + 2, os);
	right_->print(indent + 2, os);
}

void CCAPatternGraphCompareNode::print(unsigned indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "compare \'" << op_ << '\'' << std::endl;
	left_->print(indent + 2, os);
	right_->print(indent + 2, os);
}

void CCAPatternGraphCompareNode::print(unsigned indent, llvm::raw_ostream &os) const {
	os << std::string(indent, ' ') << "compare \'" << op_ << "\'\n";
	left_->print(indent + 2, os);
	right_->print(indent + 2, os);
}

void CCAPatternGraphSelectNode::print(unsigned indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "select\n";
	cmp_->print(indent + 2, os);
	os << std::string(indent + 2, ' ') << "true:\n";
	true_expr_->print(indent + 4, os);
	os << std::string(indent + 2, ' ') << "false:\n";
	false_expr_->print(indent + 4, os);
}

void CCAPatternGraphSelectNode::print(unsigned indent, llvm::raw_ostream &os) const {
	os << std::string(indent, ' ') << "select\n";
	cmp_->print(indent + 2, os);
	os << std::string(indent + 2, ' ') << "true:\n";
	true_expr_->print(indent + 4, os);
	os << std::string(indent + 2, ' ') << "false:\n";
	false_expr_->print(indent + 4, os);
}

// Ready For Search
void CCAPatternGraphRegisterNode::readyForSearch(void) {
	if (SG_ != nullptr) SG_->readyForSearch();
}

void CCAPatternGraphOperatorNode::readyForSearch(void) {
	left_->readyForSearch();
	right_->readyForSearch();
}

void CCAPatternGraphCompareNode::readyForSearch(void) {
	left_->readyForSearch();
	right_->readyForSearch();
}

void CCAPatternGraphSelectNode::readyForSearch(void) {
	cmp_->readyForSearch();
	true_expr_->readyForSearch();
	false_expr_->readyForSearch();
}

// Check Valid
bool CCAPatternGraphRegisterNode::checkValid(void) {
	if ((regtype_ == 'o' || regtype_ == 't') && SG_ == nullptr) {
		std::cerr << "[PIM-CCA-PASS][ERROR] The register \"" << regtype_ << regnum_ << "\" is not linked\n";
		return false;
	}
	return true;
}

bool CCAPatternGraphOperatorNode::checkValid(void) {
	if (!left_->checkValid()) return false;
	if (!right_->checkValid()) return false;
	return true;
}

bool CCAPatternGraphCompareNode::checkValid(void) {
	if (!left_->checkValid()) return false;
	if (!right_->checkValid()) return false;
	return true;
}

bool CCAPatternGraphSelectNode::checkValid(void) {
	if (!cmp_->checkValid()) return false;
	if (!true_expr_->checkValid()) return false;
	if (!false_expr_->checkValid()) return false;
	return true;
}

// Link Subgraph
bool CCAPatternGraphRegisterNode::linkSubgraph(char regtype, unsigned int regnum, CCAPatternSubGraph *SG) {
	if (SG_ != nullptr) return SG_->linkSubgraph(regtype, regnum, SG);
	if (SG_ == nullptr && regtype_ == regtype && regnum_ == regnum) {
		SG_ = SG;
		return true;
	}
	return false;
}

bool CCAPatternGraphOperatorNode::linkSubgraph(char regtype, unsigned int regnum, CCAPatternSubGraph *SG) {
	bool set = false;
	if (left_->linkSubgraph(regtype, regnum, SG)) set = true;
	if (right_->linkSubgraph(regtype, regnum, SG)) set = true;
	return set;
}

bool CCAPatternGraphCompareNode::linkSubgraph(char regtype, unsigned int regnum, CCAPatternSubGraph *SG) {
	bool set = false;
	if (left_->linkSubgraph(regtype, regnum, SG)) set = true;
	if (right_->linkSubgraph(regtype, regnum, SG)) set = true;
	return set;
}

bool CCAPatternGraphSelectNode::linkSubgraph(char regtype, unsigned int regnum, CCAPatternSubGraph *SG) {
	bool set = false;
	if (cmp_->linkSubgraph(regtype, regnum, SG)) set = true;
	if (true_expr_->linkSubgraph(regtype, regnum, SG)) set = true;
	if (false_expr_->linkSubgraph(regtype, regnum, SG)) set = true;
	return set;
}

// Get Flag Sizes
unsigned CCAPatternGraphRegisterNode::flagsize(void) {
	if (SG_ != nullptr) return SG_->flagsize();
	return 0;
}
unsigned CCAPatternGraphOperatorNode::flagsize(void) { return left_->flagsize() + right_->flagsize() + (reversable() ? 1 : 0); }
unsigned CCAPatternGraphCompareNode::flagsize(void) { return left_->flagsize() + right_->flagsize() + (reversable() ? 1 : 0); }
unsigned CCAPatternGraphSelectNode::flagsize(void) { return cmp_->flagsize() + true_expr_->flagsize() + false_expr_->flagsize(); }

// Set Reverse From Flag Vector
void CCAPatternGraphRegisterNode::setReverse(const std::vector<bool> &flags, unsigned int &index) {
	if (SG_ != nullptr) SG_->setReverse(flags, index);
}

void CCAPatternGraphOperatorNode::setReverse(const std::vector<bool> &flags, unsigned &index) {
	if (reversable()) {
		reversed_ = flags[index];
		index++;
	}
	left_->setReverse(flags, index);
	right_->setReverse(flags, index);
}

void CCAPatternGraphCompareNode::setReverse(const std::vector<bool> &flags, unsigned &index) {
	if (reversable()) {
		reversed_ = flags[index];
		index++;
	}
	left_->setReverse(flags, index);
	right_->setReverse(flags, index);
}

void CCAPatternGraphSelectNode::setReverse(const std::vector<bool> &flags, unsigned &index) {
	cmp_->setReverse(flags, index);
	true_expr_->setReverse(flags, index);
	false_expr_->setReverse(flags, index);
}

// Match with Codes
bool CCAPatternGraphRegisterNode::matchWithCode(Value *StartPoint,
												const std::set<Instruction *> &AlreadyRemoved,
												std::map<unsigned int, Value *> &IRVM,
												std::map<unsigned int, Value *> &ORVM,
												std::map<unsigned int, Value *> &TRVM) const {
	// Check Type
	if (StartPoint->getType() != Type::getInt32Ty(StartPoint->getContext())) return false;
	// Already Removed or Matched
	if (isa<Constant>(StartPoint)) return false;
	if (isa<Instruction>(StartPoint) && AlreadyRemoved.find(cast<Instruction>(StartPoint)) != AlreadyRemoved.end()) return false;
	// For Input Registers
	if (regtype_ == 'i') {
		if (IRVM.find(regnum_) != IRVM.end()) return IRVM.at(regnum_) == StartPoint;
		IRVM.insert({regnum_, StartPoint}); // Insert
		return true;
	}
	// For Output Register
	else if (regtype_ == 'o') {
		if (ORVM.find(regnum_) != ORVM.end()) return ORVM.at(regnum_) == StartPoint;
		if (SG_ != nullptr) {
			if (SG_->matchWithCode(StartPoint, AlreadyRemoved, IRVM, ORVM, TRVM)) {
				ORVM.insert({regnum_, StartPoint}); // Insert
				return true;
			}
		}
		return false;
	}
	// For Temporary Registers
	else if (regtype_ == 't') {
		if (TRVM.find(regnum_) != TRVM.end()) return TRVM.at(regnum_) == StartPoint;
		if (SG_ != nullptr) {
			if (SG_->matchWithCode(StartPoint, AlreadyRemoved, IRVM, ORVM, TRVM)) {
				TRVM.insert({regnum_, StartPoint}); // Insert
				return true;
			}
		}
		return false;
	}
	// Error
	else
		return false;
}

bool CCAPatternGraphOperatorNode::matchWithCode(Value *StartPoint,
												const std::set<Instruction *> &AlreadyRemoved,
												std::map<unsigned int, Value *> &IRVM,
												std::map<unsigned int, Value *> &ORVM,
												std::map<unsigned int, Value *> &TRVM) const {
	unsigned binaryOps = opcode();
	if (!llvm::isa<BinaryOperator>(StartPoint) || cast<BinaryOperator>(StartPoint)->getOpcode() != binaryOps) return false;
	// Check Matched of Child Nodes
	BinaryOperator *BO = cast<BinaryOperator>(StartPoint);
	return left_->matchWithCode(BO->getOperand(reversed_ ? 1 : 0), AlreadyRemoved, IRVM, ORVM, TRVM) &&
		   right_->matchWithCode(BO->getOperand(reversed_ ? 0 : 1), AlreadyRemoved, IRVM, ORVM, TRVM);
}

bool CCAPatternGraphCompareNode::matchWithCode(Value *StartPoint,
											   const std::set<Instruction *> &AlreadyRemoved,
											   std::map<unsigned int, Value *> &IRVM,
											   std::map<unsigned int, Value *> &ORVM,
											   std::map<unsigned int, Value *> &TRVM) const {
	if (!llvm::isa<ICmpInst>(StartPoint) || cast<ICmpInst>(StartPoint)->getPredicate() != predicate()) return false;
	// Check Matched of Child Nodes
	ICmpInst *CI = cast<ICmpInst>(StartPoint);
	return left_->matchWithCode(CI->getOperand(reversed_ ? 1 : 0), AlreadyRemoved, IRVM, ORVM, TRVM) &&
		   right_->matchWithCode(CI->getOperand(reversed_ ? 0 : 1), AlreadyRemoved, IRVM, ORVM, TRVM);
}

bool CCAPatternGraphSelectNode::matchWithCode(Value *StartPoint,
											  const std::set<Instruction *> &AlreadyRemoved,
											  std::map<unsigned int, Value *> &IRVM,
											  std::map<unsigned int, Value *> &ORVM,
											  std::map<unsigned int, Value *> &TRVM) const {
	if (!llvm::isa<SelectInst>(StartPoint)) return false;
	// Check Matched of Child Nodes
	SelectInst *SI = cast<SelectInst>(StartPoint);
	return cmp_->matchWithCode(SI->getCondition(), AlreadyRemoved, IRVM, ORVM, TRVM) &&
		   true_expr_->matchWithCode(SI->getTrueValue(), AlreadyRemoved, IRVM, ORVM, TRVM) &&
		   false_expr_->matchWithCode(SI->getFalseValue(), AlreadyRemoved, IRVM, ORVM, TRVM);
}

// Get Remove List
void CCAPatternGraphRegisterNode::getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) {
	if (SG_ != nullptr) SG_->getRemoveList(StartPoint, UserTarget, RemoveList);
	if (regtype_ == 't' && UserTarget != nullptr) {
		if (RemoveList.find(StartPoint) != RemoveList.end()) RemoveList.at(StartPoint).insert(UserTarget);
		else
			RemoveList.insert({StartPoint, {UserTarget}});
	}
}

void CCAPatternGraphOperatorNode::getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) {
	if (UserTarget != nullptr) {
		if (RemoveList.find(StartPoint) != RemoveList.end()) RemoveList.at(StartPoint).insert(UserTarget);
		else
			RemoveList.insert({StartPoint, {UserTarget}});
	}
	User *U = cast<User>(StartPoint);
	left_->getRemoveList(U->getOperand(reversed_ ? 1 : 0), U, RemoveList);
	right_->getRemoveList(U->getOperand(reversed_ ? 0 : 1), U, RemoveList);
}

void CCAPatternGraphCompareNode::getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) {
	if (UserTarget != nullptr) {
		if (RemoveList.find(StartPoint) != RemoveList.end()) RemoveList.at(StartPoint).insert(UserTarget);
		else
			RemoveList.insert({StartPoint, {UserTarget}});
	}
	User *U = cast<User>(StartPoint);
	left_->getRemoveList(U->getOperand(reversed_ ? 1 : 0), U, RemoveList);
	right_->getRemoveList(U->getOperand(reversed_ ? 0 : 1), U, RemoveList);
}

void CCAPatternGraphSelectNode::getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) {
	if (UserTarget != nullptr) {
		if (RemoveList.find(StartPoint) != RemoveList.end()) RemoveList.at(StartPoint).insert(UserTarget);
		else
			RemoveList.insert({StartPoint, {UserTarget}});
	}
	SelectInst *SI = cast<SelectInst>(StartPoint);
	cmp_->getRemoveList(SI->getCondition(), SI, RemoveList);
	true_expr_->getRemoveList(SI->getTrueValue(), SI, RemoveList);
	false_expr_->getRemoveList(SI->getFalseValue(), SI, RemoveList);
}

//-------------------------------------------
// Class: CCA Pattern Graph
//-------------------------------------------
// Constructor
CCAPatternGraph::CCAPatternGraph(unsigned rule_number, const std::vector<CCAPatternSubGraph *> SubGraphs)
	: rule_number_(rule_number), graphs_(SubGraphs), linked_graphs_(SubGraphs) {
	for (auto iter = linked_graphs_.begin(); iter != linked_graphs_.end();) {
		CCAPatternSubGraph *&SG = *iter;
		bool merged = false;
		for (auto it = linked_graphs_.begin(); it != linked_graphs_.end(); ++it) {
			(*it)->readyForSearch();
			merged = merged || (*it)->linkSubgraph(SG->regtype(), SG->regnum(), SG);
		}
		if (merged) iter = linked_graphs_.erase(iter);
		else
			++iter;
	}
	if (linked_graphs_.empty()) std::cerr << "[PIM-CCA-PASS][ERROR] There is circular linking of registers in the rule " << rule_number << '\n';
	for (auto iter = linked_graphs_.begin(); iter != linked_graphs_.end(); ++iter) {
		CCAPatternSubGraph *&SG = *iter;
		if (SG->regtype() == 't') std::cerr << "[PIM-CCA-PASS][ERROR] The temporary register \"" << SG->regnum() << "\" are not used in the rule\n";
	}
	for (auto iter = linked_graphs_.begin(); iter != linked_graphs_.end(); ++iter) {
		CCAPatternSubGraph *&SG = *iter;
		SG->readyForSearch();
		SG->checkValid();
	}
}

// Print
void CCAPatternGraph::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "pattern graph for cca " << rule_number_ << '\n';
	for (auto &SG : linked_graphs_) SG->print(indent, os);
}

void CCAPatternGraph::print(unsigned int indent, llvm::raw_ostream &os) const {
	os << std::string(indent, ' ') << "pattern graph for cca " << rule_number_ << '\n';
	for (auto &SG : linked_graphs_) SG->print(indent, os);
}

// Match With Codes
bool CCAPatternGraph::matchWithCode(const std::vector<Instruction *> &Candidate,
									const std::set<Instruction *> &UnRemovable,
									std::set<Instruction *> &Removed,
									std::map<unsigned, Value *> &InputRegValueMap,
									std::map<unsigned, Value *> &OutputRegValueMap) const {
	std::vector<unsigned> flagsize;
	for (auto &G : linked_graphs_) {
		G->readyForSearch();
		flagsize.push_back(G->flagsize());
	}
	unsigned flagend = std::accumulate(flagsize.begin(), flagsize.end(), 0);

	for (Instruction *I : Candidate)
		if (UnRemovable.find(I) != UnRemovable.end()) return false;

	for (unsigned flag = 0; flag < (0x1 << flagend); ++flag) {
		std::map<unsigned, Value *> IRVM, ORVM(OutputRegValueMap), TRVM;
		std::map<Value *, std::set<User *>> RL;

		unsigned matched = true;
		for (unsigned gidx = 0; gidx < linked_graphs_.size(); ++gidx) {
			CCAPatternSubGraph *G = linked_graphs_[gidx];
			Instruction *StartPoint = Candidate[gidx];
			// Set Reversed
			std::vector<bool> reversed(flagsize[gidx], false);
			unsigned partialflag = (flag >> std::accumulate(flagsize.begin(), flagsize.begin() + gidx, 0));
			for (unsigned fidx = 0; fidx < flagsize[gidx]; ++fidx) reversed[fidx] = (partialflag & (0x1 << fidx)) ? true : false;
			unsigned index = 0;
			G->readyForSearch();
			G->setReverse(reversed, index);
			// Match with Code
			if (!G->matchWithCode(StartPoint, Removed, IRVM, ORVM, TRVM)) {
				matched = false;
				break;
			}
			// Get Remove List
			G->readyForSearch();
			G->getRemoveList(StartPoint, nullptr, RL);
		}
		if (!matched) continue;
		// Check Remove Lists
		std::set<Instruction *> RIL;
		BasicBlock *parent = nullptr;
		for (auto &mapIter : RL) {
			// if(!isa<Instruction>(mapIter.first)) /* error */
			Instruction *I = cast<Instruction>(mapIter.first);
			// Check Instructions are Removable
			if (UnRemovable.find(I) != UnRemovable.end()) {
				matched = false;
				break;
			}
			// Check Instructions came from same Parent
			if (parent == nullptr) parent = I->getParent();
			else if (parent != I->getParent()) {
				matched = false;
				break;
			}
			// Check Users of Instructions to be Removed
			for (auto UserIter : I->users()) {
				if (mapIter.second.find(UserIter) != mapIter.second.end()) continue;
				if (isa<StoreInst>(UserIter)) {
					// Check Other Store Exists after This
					StoreInst *S = cast<StoreInst>(UserIter);
					bool removableStore = false;
					for (auto SPIter = S->getIterator(); SPIter != parent->end(); ++SPIter) {
						if (!isa<StoreInst>(SPIter)) continue;
						StoreInst *S2 = cast<StoreInst>(SPIter);
						if (S != S2 && S->getPointerOperand() == S2->getPointerOperand()) {
							removableStore = true;
							break;
						}
					}
					if (removableStore) {
						RIL.insert(cast<Instruction>(UserIter));
						continue;
					}
				}
				matched = false;
				break;
			}
			RIL.insert(I);
		}
		if (!matched) continue;
		// Check Output Registers
		for (auto &mapIter : ORVM) {
			Instruction *I = cast<Instruction>(mapIter.second);
			if (UnRemovable.find(I) != UnRemovable.end()) {
				matched = false;
				break;
			}
		}
		if (!matched) continue;
		// Return
		InputRegValueMap = IRVM;
		OutputRegValueMap = ORVM;
		Removed.insert(RIL.begin(), RIL.end());
		return matched;
	}
	return false;
}

} // namespace cca
} // namespace llvm
