#include "Instrumentation/CCAUniversal.hpp"
#include "Instrumentation/CCAPatternGraph.hpp"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_os_ostream.h"
#include <iostream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;

namespace llvm {
namespace cca {

// Print Pattern Instance
void CCAPattern::print(unsigned indent, std::ostream &os) const {
	// output
	llvm::raw_os_ostream los(os);
	los << std::string(indent, ' ') << "output : ";
	O_->print(los);
	los << '\n';

	for (const auto &iter : InputRegValueMap_) {
		los << std::string(indent + 2, ' ') << "input " << iter.first << " : ";
		iter.second->print(los);
		los << '\n';
	}
	los.flush();
}

void CCAPattern2::print(unsigned indent, std::ostream &os) const {
	// output
	llvm::raw_os_ostream los(os);
	for (auto *I : C_) {
		los << std::string(indent, ' ') << "output : ";
		I->print(los);
		los << '\n';
	}
	// input
	for (const auto &iter : InputRegValueMap_) {
		los << std::string(indent + 2, ' ') << "input " << iter.first << " : ";
		iter.second->print(los);
		los << '\n';
	}
	los.flush();
}

// Build (Find) CCA Pattern in the Codes with a Pattern Graph Instance
CCAPattern *CCAPattern::get(CCAPatternGraph *Graph,
							Instruction *StartPoint,
							std::set<Instruction *> &Removed,
							const std::set<Instruction *> &Replaced) {
	CCAPattern *P = new CCAPattern(StartPoint);
	if (!Graph->matchWithCode(StartPoint, Removed, Replaced, P->InputRegValueMap_)) {
		delete P;
		P = nullptr;
	}
	return P;
}

CCAPattern2 *CCAPattern2::get(CCAPatternGraph2 *Graph,
							  const std::vector<Instruction *> &Candidate,
							  std::set<Instruction *> &Removed,
							  const std::set<Instruction *> &UnRemovable) {

	CCAPattern2 *P = new CCAPattern2(Candidate);
	if (!Graph->matchWithCode(Candidate, UnRemovable, Removed, P->InputRegValueMap_)) {
		delete P;
		P = nullptr;
	}
	return P;
}

// Build CCA Instruction from Matched Patterns
void CCAPattern::build(unsigned int ccaid, unsigned int oregnum, char oregtype, LLVMContext &Context) {
	Type *VoidTy = Type::getVoidTy(Context);
	Type *Int32Ty = Type::getInt32Ty(Context);

	// Prepare CCA Inline Assembly
	FunctionType *MoveInputRegInstFT = FunctionType::get(VoidTy, {Int32Ty}, false);
	std::map<unsigned int, InlineAsm *> MoveInputRegInstIAMap;
	for (const auto &mapIter : InputRegValueMap_) {
		unsigned int regnum = mapIter.first;
		MoveInputRegInstIAMap.insert(
			{regnum, InlineAsm::get(MoveInputRegInstFT, "#removethiscomment move r" + std::to_string(regnum) + ", $0", "r", true)});
	}

	FunctionType *CCAInstFT = FunctionType::get(VoidTy, false);
	InlineAsm *CCAInstIA = InlineAsm::get(CCAInstFT, "#removethiscomment cca " + std::to_string(ccaid), "", true);

	FunctionType *MoveOutputRegInstFT = FunctionType::get(Int32Ty, false);
	InlineAsm *MoveOutputRegInstIA = InlineAsm::get(MoveOutputRegInstFT, "#removethiscomment move $0, r" + std::to_string(oregnum), "=r", true);

	// Build Instructions
	// Move Input Value to Register
	std::vector<Instruction *> MoveInputRegInstVec;
	for (const auto &mapIter : InputRegValueMap_) {
		CallInst *MoveInputRegInst = CallInst::Create(FunctionCallee(MoveInputRegInstFT, MoveInputRegInstIAMap.at(mapIter.first)), mapIter.second);
		MoveInputRegInst->setTailCall(true);
		MoveInputRegInstVec.push_back(MoveInputRegInst);
	}
	// Run CCA
	CallInst *CCACallInst = CallInst::Create(FunctionCallee(CCAInstFT, CCAInstIA));
	CCACallInst->setTailCall(true);
	// Move Output Value to Register
	CallInst *MoveOutputRegInst = CallInst::Create(FunctionCallee(MoveOutputRegInstFT, MoveOutputRegInstIA));
	MoveOutputRegInst->insertBefore(O_);
	O_->replaceAllUsesWith(MoveOutputRegInst);
	CCACallInst->insertBefore(MoveOutputRegInst);
	for (const auto &vecIter : MoveInputRegInstVec) vecIter->insertBefore(CCACallInst);
}

void CCAPattern2::build(unsigned int ccaid, const std::vector<std::pair<unsigned int, char>> &oreginfo, LLVMContext &Context) {
	Type *VoidTy = Type::getVoidTy(Context);
	Type *Int32Ty = Type::getInt32Ty(Context);

	// Prepare CCA Inline Assembly
	FunctionType *MoveInputRegInstFT = FunctionType::get(VoidTy, {Int32Ty}, false);
	std::map<unsigned int, InlineAsm *> MoveInputRegInstIAMap;
	for (const auto &mapIter : InputRegValueMap_) {
		unsigned int regnum = mapIter.first;
		MoveInputRegInstIAMap.insert(
			{regnum, InlineAsm::get(MoveInputRegInstFT, "#removethiscomment move r" + std::to_string(regnum) + ", $0", "r", true)});
	}

	FunctionType *CCAInstFT = FunctionType::get(VoidTy, false);
	InlineAsm *CCAInstIA = InlineAsm::get(CCAInstFT, "#removethiscomment cca " + std::to_string(ccaid), "", true);

	FunctionType *MoveOutputRegInstFT = FunctionType::get(Int32Ty, false);
	std::map<unsigned int, InlineAsm *> MoveOutputRegInstIAMap;
	for (const auto &mapIter : oreginfo) {
		unsigned int regnum = mapIter.first;
		MoveOutputRegInstIAMap.insert(
			{regnum, InlineAsm::get(MoveOutputRegInstFT, "#removethiscomment move $0, r" + std::to_string(regnum), "=r", true)});
	}

	// Build Instructions
	// Move Input Value to Register
	std::vector<Instruction *> MoveInputRegInstVec;
	for (const auto &mapIter : InputRegValueMap_) {
		CallInst *MoveInputRegInst = CallInst::Create(FunctionCallee(MoveInputRegInstFT, MoveInputRegInstIAMap.at(mapIter.first)), mapIter.second);
		MoveInputRegInst->setTailCall(true);
		MoveInputRegInstVec.push_back(MoveInputRegInst);
	}
	// Run CCA
	CallInst *CCACallInst = CallInst::Create(FunctionCallee(CCAInstFT, CCAInstIA));
	CCACallInst->setTailCall(true);
	// Move Output Value to Register
	std::vector<Instruction *> MoveOutputRegInstVec;
	for (unsigned idx = 0; idx < C_.size(); ++idx) {
		CallInst *MoveOutputRegInst = CallInst::Create(FunctionCallee(MoveOutputRegInstFT, MoveOutputRegInstIAMap.at(oreginfo[idx].first)));
		MoveOutputRegInst->setTailCall(false);
		MoveOutputRegInstVec.push_back(MoveOutputRegInst);
	}

	// Insert Instructions & Replace All Uses
	Instruction *InsertPos = C_.at(0);
	for (auto *I : C_)
		if (I->comesBefore(InsertPos)) InsertPos = I;
	for (const auto &vecIter : MoveInputRegInstVec) vecIter->insertBefore(InsertPos);
	CCACallInst->insertBefore(InsertPos);
	for (unsigned idx = 0; idx < C_.size(); ++idx) {
		MoveOutputRegInstVec[idx]->insertBefore(InsertPos);
		C_[idx]->replaceAllUsesWith(MoveOutputRegInstVec[idx]);
	}
}

// Pass Constructor
CCAUniversalPass::CCAUniversalPass(std::string patternStr) {
	// Parse Input String
	std::vector<std::string> tokenVec;
	std::stringstream ss(patternStr);
	std::string line;
	while (getline(ss, line)) {
		size_t prev = 0, pos;
		while ((pos = line.find_first_of(" =+*/-:()", prev)) != std::string::npos) {
			if (pos > prev) tokenVec.push_back(line.substr(prev, pos - prev));
			if (line[pos] != ' ') tokenVec.push_back(std::string(1, line[pos]));
			prev = pos + 1;
		}
		if (prev < line.length()) tokenVec.push_back(line.substr(prev, std::string::npos));
	}

	// Check Rule Number
	// [0] : CCA ID
	// [1] : colon (':')
	if (tokenVec.size() < 2 || tokenVec.at(1) != ":") std::cerr << "undefined pattern string : colon not detected in 2nd token" << std::endl;
	ccaid_ = std::atoi(tokenVec.at(0).c_str());

	// Check Assignment
	// [2] : output register (r30)
	// [3] : assignment ('=')
	if (tokenVec.size() < 4 || tokenVec.at(3) != "=") std::cerr << "undefined pattern string : assignment not detected in 2nd token" << std::endl;
	oregtype_ = tokenVec.at(2).at(0);
	oregnum_ = std::atoi(tokenVec.at(2).substr(1, std::string::npos).c_str());

	// Build Pattern Graph from Pattern STring
	G_ = BuildGraph(std::vector<std::string>(tokenVec.begin() + 4, tokenVec.end()));

	// Verbose
	std::cout << "[PASS] Build Pass from \"" << patternStr << '\"' << std::endl;
	std::cout << std::string(2, ' ') << "output register \'" << oregnum_ << "\' (type " << oregtype_ << ')' << std::endl;
	G_->print(4, std::cout);
}

// Pass Run
PreservedAnalyses CCAUniversalPass::run(Function &F, FunctionAnalysisManager &) {
	std::set<Instruction *> IntermediateInsts;
	std::set<Instruction *> ReplacedInsts;

	// Find Patterns using Graph
	std::vector<CCAPattern *> PatternVec;
	for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
		for (BasicBlock::iterator BBIter = FuncIter->begin(); BBIter != FuncIter->end(); ++BBIter) {
			Instruction *I = cast<Instruction>(BBIter);
			bool alreadyRemovedInsts = IntermediateInsts.find(I) != IntermediateInsts.end();
			CCAPattern *P = CCAPattern::get(G_, I, IntermediateInsts, ReplacedInsts);
			if (P != nullptr) {
				PatternVec.push_back(P);
				if (!alreadyRemovedInsts) IntermediateInsts.erase(I);
				ReplacedInsts.insert(I);
			}
		}
	}

	// Verbose
	if (!PatternVec.empty()) {
		raw_os_ostream lcout(std::cout);
		lcout << "[PASS] Found Patterns in Function " << F.getName() << '\n';
		lcout.flush();
		for (auto &P : PatternVec) P->print(2, std::cout);
		lcout << "  - removed: \n";
		for (const auto &iter : IntermediateInsts) {
			lcout << std::string(4, ' ');
			iter->print(lcout);
			lcout << '\n';
		}
		lcout.flush();
		lcout << "  - replaced: \n";
		for (const auto &iter : ReplacedInsts) {
			lcout << std::string(4, ' ');
			iter->print(lcout);
			lcout << '\n';
		}
		lcout.flush();
	}

	// Build CCA Instructions from Patterns
	for (auto &P : PatternVec) P->build(ccaid_, oregnum_, oregtype_, F.getContext());

	// Remove Intermediate Instructions
	for (auto &I : ReplacedInsts) I->eraseFromParent();
	bool changed = true;
	while (changed) {
		changed = false;
		std::vector<Instruction *> Removable;
		for (auto &I : IntermediateInsts) {
			if (I->users().empty()) Removable.push_back(I);
		}
		for (auto &I : Removable) {
			IntermediateInsts.erase(I);
			I->eraseFromParent();
			changed = true;
		}
	}

	if (!IntermediateInsts.empty()) {
		raw_os_ostream lcout(std::cout);
		lcout << "[PASS] Cannot Resolve All the Intermediate Instructions\n";
		lcout.flush();
		for (const auto &I : IntermediateInsts) {
			I->print(lcout);
			lcout << '\n';
			for (const auto &V : I->users()) {
				lcout << "  - ";
				V->print(lcout);
				lcout << '\n';
			}
		}
	}

	return PreservedAnalyses::all();
}

// Pass Constructor
CCAUniversalPass2::CCAUniversalPass2(std::string patternStr) {
	// Parse Input String
	std::vector<std::string> tokenVec;
	std::stringstream ss(patternStr);
	std::string line;
	while (getline(ss, line)) {
		size_t prev = 0, pos;
		while ((pos = line.find_first_of(" =+*/-:();", prev)) != std::string::npos) {
			if (pos > prev) tokenVec.push_back(line.substr(prev, pos - prev));
			if (line[pos] != ' ') tokenVec.push_back(std::string(1, line[pos]));
			prev = pos + 1;
		}
		if (prev < line.length()) tokenVec.push_back(line.substr(prev, std::string::npos));
	}

	// Check Rule Number
	// [0] : CCA ID
	// [1] : colon (':')
	if (tokenVec.size() < 2 || tokenVec.at(1) != ":") std::cerr << "undefined pattern string : colon not detected in 2nd token" << std::endl;
	ccaid_ = std::atoi(tokenVec.at(0).c_str());

	// Separate by Semicolon
	unsigned prev = 1, pos = prev;
	std::vector<CCAPatternGraphNode2 *> SubGraphs;
	while (pos < tokenVec.size()) {
		// Find Semicolon
		while (++pos < tokenVec.size() && tokenVec.at(pos) != ";");
		// Check Assignment
		// [prev] : semicolon (';')
		// [prev+1] : output register
		// [prev+2] : assignement ('=')
		// [prev+3~pos] : math expression
		// [pos] : semicolon (';')
		if (pos < prev + 1 || tokenVec.at(prev + 2) != "=")
			std::cerr << "undefined pattern string : assignment not detected in 2nd token" << std::endl;
		// Build Pattern Graph from Pattern String
		char oregtype = tokenVec.at(prev + 1).at(0);
		unsigned int oregnum = std::atoi(tokenVec.at(prev + 1).substr(1, std::string::npos).c_str());
		oreginfo_.push_back({oregnum, oregtype});

		std::cout << std::endl;
		CCAPatternGraphNode2 *SG = BuildGraph2(std::vector<std::string>(tokenVec.begin() + prev + 3, tokenVec.begin() + pos));
		SubGraphs.push_back(SG);

		// Update prev
		prev = pos;
	}
	G_ = new CCAPatternGraph2();
	G_->setGraph(SubGraphs);
	// Verbose
	std::cout << "[PASS] Build Pass from \"" << patternStr << '\"' << std::endl;
	for (unsigned idx = 0; idx < oreginfo_.size(); ++idx) {
		std::cout << std::string(2, ' ') << "output register \'" << oreginfo_.at(idx).first << "\' (type " << oreginfo_.at(idx).second << ')'
				  << std::endl;
		G_->print(4, idx, std::cout);
	}
}

// Pass Run
PreservedAnalyses CCAUniversalPass2::run(Function &F, FunctionAnalysisManager &) {
	// Get Seed Instructions
	std::vector<BinaryOperator *> AddInst;
	std::vector<BinaryOperator *> SubInst;
	std::vector<BinaryOperator *> MulInst;
	std::vector<BinaryOperator *> UDivInst;

	for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
		for (BasicBlock::iterator BBIter = FuncIter->begin(); BBIter != FuncIter->end(); ++BBIter) {
			if (!isa<BinaryOperator>(BBIter)) continue;
			BinaryOperator *I = cast<BinaryOperator>(BBIter);
			switch (I->getOpcode()) {
			case Instruction::BinaryOps::Add: AddInst.push_back(I); break;
			case Instruction::BinaryOps::Sub: SubInst.push_back(I); break;
			case Instruction::BinaryOps::Mul: MulInst.push_back(I); break;
			case Instruction::BinaryOps::UDiv: UDivInst.push_back(I); break;
			default:;
			}
		}
	}
	raw_os_ostream lcout(std::cout);

	// Find Patterns
	std::set<Instruction *> RemovedInsts;
	std::set<Instruction *> ReplacedInsts;
	std::vector<CCAPattern2 *> PatternVec;

	std::vector<std::vector<BinaryOperator *>::iterator> IterVec;
	std::vector<std::vector<BinaryOperator *>::iterator> IterBeginVec;
	std::vector<std::vector<BinaryOperator *>::iterator> IterEndVec;
	auto BOVec = G_->getBOS();
	for (auto BO : BOVec) {
		switch (BO) {
		case Instruction::BinaryOps::Add:
			IterVec.push_back(AddInst.begin());
			IterBeginVec.push_back(AddInst.begin());
			IterEndVec.push_back(AddInst.end());
			break;
		case Instruction::BinaryOps::Sub:
			IterVec.push_back(SubInst.begin());
			IterBeginVec.push_back(SubInst.begin());
			IterEndVec.push_back(SubInst.end());
			break;
		case Instruction::BinaryOps::Mul:
			IterVec.push_back(MulInst.begin());
			IterBeginVec.push_back(MulInst.begin());
			IterEndVec.push_back(MulInst.end());
			break;
		case Instruction::BinaryOps::UDiv:
			IterVec.push_back(UDivInst.begin());
			IterBeginVec.push_back(MulInst.begin());
			IterEndVec.push_back(UDivInst.end());
			break;
		default: break; /* error */
		}
	}

	auto updateIter = [&](void) -> bool {
		for (unsigned idx = 0; idx < IterVec.size(); ++idx) {
			IterVec.at(idx)++;
			if (IterVec.at(idx) != IterEndVec.at(idx)) return true;
			IterVec.at(idx) = IterBeginVec.at(idx);
		}
		return false;
	};

	auto printIter = [&](void) -> void {
		for (auto &it : IterVec) {
			(*it)->print(lcout);
			lcout << '\n';
		}
		lcout << '\n';
	};

	auto checkIterDuplicated = [&](void) -> bool {
		for (unsigned i = 0; i < IterVec.size(); ++i) {
			for (unsigned j = 0; j < IterVec.size(); ++j)
				if (i != j && IterVec.at(i) == IterVec.at(j)) { return true; }
		}
		return false;
	};

	bool terminated = false;
	while (!terminated) {
		// Update Until Not Duplicated
		while (!terminated && checkIterDuplicated()) terminated = !updateIter();
		if (terminated) break;

		// Get Patterns using Candidates
		std::vector<Instruction *> Candidate;
		for (auto &IterVecIter : IterVec) Candidate.push_back(*IterVecIter);
		CCAPattern2 *P = CCAPattern2::get(G_, Candidate, RemovedInsts, ReplacedInsts);
		if (P != nullptr) {
			PatternVec.push_back(P);
			ReplacedInsts.insert(Candidate.begin(), Candidate.end());
		}
		// Update Iterators
		terminated = !updateIter();
	}

	// Verbose
	if (!PatternVec.empty()) {
		raw_os_ostream lcout(std::cout);
		lcout << "[PASS] Found Patterns in Function " << F.getName() << '\n';
		lcout.flush();
		for (auto &P : PatternVec) P->print(2, std::cout);
		lcout << "  - removed: \n";
		for (const auto &iter : RemovedInsts) {
			lcout << std::string(4, ' ');
			iter->print(lcout);
			lcout << '\n';
		}
		lcout.flush();
		lcout << "  - replaced: \n";
		for (const auto &iter : ReplacedInsts) {
			lcout << std::string(4, ' ');
			iter->print(lcout);
			lcout << '\n';
		}
		lcout.flush();
	}

	// Build CCA Instructions from Patterns
	for (auto &P : PatternVec) P->build(ccaid_, oreginfo_, F.getContext());

	// Remove Intermediate Instructions
	for (auto &I : ReplacedInsts) I->eraseFromParent();
	bool changed = true;
	while (changed) {
		changed = false;
		std::vector<Instruction *> Removable;
		for (auto &I : RemovedInsts) {
			if (I->users().empty()) Removable.push_back(I);
		}
		for (auto &I : Removable) {
			RemovedInsts.erase(I);
			I->eraseFromParent();
			changed = true;
		}
	}

	if (!RemovedInsts.empty()) {
		raw_os_ostream lcout(std::cout);
		lcout << "[PASS] Cannot Resolve All the Intermediate Instructions\n";
		lcout.flush();
		for (const auto &I : RemovedInsts) {
			I->print(lcout);
			lcout << '\n';
			for (const auto &V : I->users()) {
				lcout << "  - ";
				V->print(lcout);
				lcout << '\n';
			}
		}
	}

	return PreservedAnalyses::all();
}

} // namespace cca
} // namespace llvm
