#include "Instrumentation/CCAPatternGraph.hpp"
#include "Instrumentation/Utils.hpp"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include <iostream>
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

} // namespace cca
} // namespace llvm
