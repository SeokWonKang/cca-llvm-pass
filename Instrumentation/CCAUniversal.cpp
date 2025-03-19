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

} // namespace cca
} // namespace llvm
