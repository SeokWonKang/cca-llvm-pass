#include "Instrumentation/CCAPatternGraph.hpp"
#include "Instrumentation/Utils.hpp"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <stack>
#include <string>
#include <vector>

namespace llvm {
namespace cca {

// Build Graph from String
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

inline CCAPatternGraph *BuildCCAPatternGraphFromPostfixTokens(const std::vector<std::string> &PostfixTokenVec) {
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

CCAPatternGraph *BuildGraph(const std::vector<std::string> &patternTokVec) {
	return BuildCCAPatternGraphFromPostfixTokens(ConvertInfixToPostfix(patternTokVec));
}

inline CCAPatternGraphNode2 *BuildCCAPatternGraph2FromPostfixTokens(const std::vector<std::string> &PostfixTokenVec) {
	std::stack<CCAPatternGraphNode2 *> st;
	for (auto &tok : PostfixTokenVec) {
		if (isOpTok(tok)) {
			CCAPatternGraphNode2 *rightNode = st.top();
			st.pop();
			CCAPatternGraphNode2 *leftNode = st.top();
			st.pop();
			CCAPatternGraphNode2 *opNode = new CCAPatternGraphOperatorNode2(tok[0], leftNode, rightNode);
			st.push(opNode);
		} else
			st.push(new CCAPatternGraphRegisterNode2(tok));
	}
	return st.top();
}

CCAPatternGraphNode2 *BuildGraph2(const std::vector<std::string> &patternTokVec) {
	return BuildCCAPatternGraph2FromPostfixTokens(ConvertInfixToPostfix(patternTokVec));
}

//---------------------------------
// CCA Pattern Graph
//---------------------------------
// Print Graph
void CCAPatternGraphOperatorNode::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "operator \'" << opchar_ << '\'' << std::endl;
	left_->print(indent + 2, os);
	right_->print(indent + 2, os);
}

void CCAPatternGraphRegisterNode::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "register \'" << regnum_ << "\' (type " << regtype_ << ')' << std::endl;
}

// Match and Parse Codes
bool CCAPatternGraphOperatorNode::matchWithCode(Value *StartPoint,
												std::set<Instruction *> &Removed,
												const std::set<Instruction *> &Replaced,
												std::map<unsigned int, Value *> &InputRegValueMap) const {
	Instruction::BinaryOps binaryOps;
	switch (opchar_) {
	case '+': binaryOps = Instruction::Add; break;
	case '-': binaryOps = Instruction::Sub; break;
	case '*': binaryOps = Instruction::Mul; break;
	case '/': binaryOps = Instruction::UDiv; break;
	default: binaryOps = Instruction::BinaryOpsEnd;
	}
	if (binaryOps == Instruction::BinaryOpsEnd) return false; /* error */
	if (!isaBO(StartPoint, binaryOps)) return false;
	BinaryOperator *BO = cast<BinaryOperator>(StartPoint);
	std::vector<StoreInst *> SVec;
	// TODO: Tertiary Store with Intermediate Results
	if (CheckOtherUseExistWithoutStore(BO->getOperand(0), BO, SVec)) return false;
	if (CheckOtherUseExistWithoutStore(BO->getOperand(1), BO, SVec)) return false;
	{
		std::set<Instruction *> TempRemoved(Removed);
		std::map<unsigned int, Value *> TempInputRegValueMap(InputRegValueMap);
		bool matched = (left_->matchWithCode(BO->getOperand(0), TempRemoved, Replaced, TempInputRegValueMap) &&
						right_->matchWithCode(BO->getOperand(1), TempRemoved, Replaced, TempInputRegValueMap));
		if (matched) {
			Removed = TempRemoved;
			Removed.insert(SVec.begin(), SVec.end());
			if (Replaced.find(BO) == Replaced.end()) Removed.insert(BO);
			InputRegValueMap = TempInputRegValueMap;
			return true;
		}
	}
	if (opchar_ == '+' || opchar_ == '*') {
		std::set<Instruction *> TempRemoved(Removed);
		std::map<unsigned int, Value *> TempInputRegValueMap(InputRegValueMap);
		bool matched = (left_->matchWithCode(BO->getOperand(1), TempRemoved, Replaced, TempInputRegValueMap) &&
						right_->matchWithCode(BO->getOperand(0), TempRemoved, Replaced, TempInputRegValueMap));
		if (matched) {
			Removed = TempRemoved;
			Removed.insert(SVec.begin(), SVec.end());
			if (Replaced.find(BO) == Replaced.end()) Removed.insert(BO);
			InputRegValueMap = TempInputRegValueMap;
			return true;
		}
	}
	return false;
}

bool CCAPatternGraphRegisterNode::matchWithCode(Value *StartPoint,
												std::set<Instruction *> &Removed,
												const std::set<Instruction *> &Replaced,
												std::map<unsigned int, Value *> &InputRegValueMap) const {
	if (InputRegValueMap.find(regnum_) != InputRegValueMap.end() && InputRegValueMap.at(regnum_) == StartPoint) return true;
	if (isa<Instruction>(StartPoint) && Removed.find(cast<Instruction>(StartPoint)) != Removed.end()) return false;
	InputRegValueMap.insert({regnum_, StartPoint});
	return true;
}

//-------------------------------------
// Class: CCA Pattern Graph 2
//-------------------------------------

// Print Graph
void CCAPatternGraphOperatorNode2::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "operator \'" << opchar_ << '\'' << std::endl;
	left_->print(indent + 2, os);
	right_->print(indent + 2, os);
}

void CCAPatternGraphRegisterNode2::print(unsigned int indent, std::ostream &os) const {
	os << std::string(indent, ' ') << "register \'" << regnum_ << "\' (type " << regtype_ << ')' << std::endl;
}

void CCAPatternGraph2::print(unsigned int indent, unsigned int gidx, std::ostream &os) const { graphs_.at(gidx)->print(indent, os); }

// Get Flag Sizes
unsigned CCAPatternGraphOperatorNode2::flagsize(void) const {
	return left_->flagsize() + right_->flagsize() + ((opchar_ == '+' || opchar_ == '*') ? 1 : 0);
}

// Set Reverse From Flag Vector
void CCAPatternGraphOperatorNode2::reverse(const std::vector<bool> &flags, unsigned &index) {
	if (opchar_ == '+' || opchar_ == '*') {
		reversed_ = flags[index];
		index++;
	}
	left_->reverse(flags, index);
	right_->reverse(flags, index);
}

// Match with Codes
bool CCAPatternGraphOperatorNode2::matchWithCode(Value *StartPoint,
												 const std::set<Instruction *> &AlreadyRemoved,
												 std::map<unsigned int, Value *> &InputRegValueMap) const {
	Instruction::BinaryOps binaryOps;
	// Check Opcode
	switch (opchar_) {
	case '+': binaryOps = Instruction::Add; break;
	case '-': binaryOps = Instruction::Sub; break;
	case '*': binaryOps = Instruction::Mul; break;
	case '/': binaryOps = Instruction::UDiv; break;
	default: binaryOps = Instruction::BinaryOpsEnd;
	}
	if (binaryOps == Instruction::BinaryOpsEnd) return false; /* error */
	if (!isaBO(StartPoint, binaryOps)) return false;

	// Check Matched of Child Nodes
	BinaryOperator *BO = cast<BinaryOperator>(StartPoint);
	return left_->matchWithCode(BO->getOperand(reversed_ ? 1 : 0), AlreadyRemoved, InputRegValueMap) &&
		   right_->matchWithCode(BO->getOperand(reversed_ ? 0 : 1), AlreadyRemoved, InputRegValueMap);
}

bool CCAPatternGraphRegisterNode2::matchWithCode(Value *StartPoint,
												 const std::set<Instruction *> &AlreadyRemoved,
												 std::map<unsigned int, Value *> &InputRegValueMap) const {
	// Already Removed or Matched
	if (isa<Constant>(StartPoint)) return false;
	if (isa<Instruction>(StartPoint) && AlreadyRemoved.find(cast<Instruction>(StartPoint)) != AlreadyRemoved.end()) return false;
	if (InputRegValueMap.find(regnum_) != InputRegValueMap.end()) {
		if (InputRegValueMap.at(regnum_) == StartPoint) return true;
		else
			return false;
	}
	// Insert to Register Map
	InputRegValueMap.insert({regnum_, StartPoint});
	// Return Matched
	return true;
}

// Get Remove List
void CCAPatternGraphOperatorNode2::getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) const {
	if (UserTarget != nullptr) {
		if (RemoveList.find(StartPoint) != RemoveList.end()) RemoveList.at(StartPoint).insert(UserTarget);
		else
			RemoveList.insert({StartPoint, {UserTarget}});
	}
	User *U = cast<User>(StartPoint);
	left_->getRemoveList(U->getOperand(reversed_ ? 1 : 0), U, RemoveList);
	right_->getRemoveList(U->getOperand(reversed_ ? 1 : 0), U, RemoveList);
}

void CCAPatternGraphRegisterNode2::getRemoveList(Value *StartPoint, User *UserTarget, std::map<Value *, std::set<User *>> &RemoveList) const {}

// Match With Codes
bool CCAPatternGraph2::matchWithCode(const std::vector<Instruction *> &Candidate,
									 const std::set<Instruction *> &UnRemovable,
									 std::set<Instruction *> &Removed,
									 std::map<unsigned, Value *> &InputRegValueMap) const {
	std::vector<unsigned> flagsize;
	for (auto &G : graphs_) flagsize.push_back(G->flagsize());
	unsigned flagend = std::accumulate(flagsize.begin(), flagsize.end(), 0);

	for (unsigned flag = 0; flag < (0x1 << flagend); ++flag) {
		std::map<unsigned, Value *> IRVM;
		std::map<Value *, std::set<User *>> RL;

		unsigned matched = true;
		for (unsigned gidx = 0; gidx < graphs_.size(); ++gidx) {
			CCAPatternGraphNode2 *G = graphs_[gidx];
			Instruction *StartPoint = Candidate[gidx];
			// Set Reversed
			std::vector<bool> reversed(flagsize[gidx], false);
			unsigned partialflag = (flag >> std::accumulate(flagsize.begin(), flagsize.begin() + gidx, 0));
			for (unsigned fidx = 0; fidx < flagsize[gidx]; ++fidx) reversed[fidx] = (partialflag & (0x1 << fidx)) ? true : false;
			unsigned t = 0;
			G->reverse(reversed, t);

			// Match with Code
			if (!G->matchWithCode(StartPoint, Removed, IRVM)) {
				matched = false;
				break;
			}
			// Get Remove List
			G->getRemoveList(StartPoint, nullptr, RL);
		}
		if (!matched) continue;
		// Check Remove Lists & Build
		std::set<Instruction *> RIL;
		for (auto &mapIter : RL) {
			// if(!isa<Instruction>(mapIter.first)) /* error */
			Instruction *I = cast<Instruction>(mapIter.first);
			// Check Instructions are Removable
			if (UnRemovable.find(I) != UnRemovable.end()) {
				matched = false;
				break;
			}
			// Check Users of Instructions to be Removed
			for (auto UserIter : I->users()) {
				if (mapIter.second.find(UserIter) != mapIter.second.end()) continue;
				if (isa<StoreInst>(UserIter)) {
					RIL.insert(cast<Instruction>(UserIter));
					continue;
				}
				matched = false;
				break;
			}
			RIL.insert(I);
		}
		if (!matched) continue;
		// Return
		InputRegValueMap = IRVM;
		Removed.insert(RIL.begin(), RIL.end());
		return matched;
	}
	return false;
}

} // namespace cca
} // namespace llvm
