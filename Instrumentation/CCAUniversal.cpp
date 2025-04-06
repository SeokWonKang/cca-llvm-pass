#include "Instrumentation/CCAUniversal.hpp"
#include "Instrumentation/CCAPatternGraph.hpp"
#include "Instrumentation/parser/parser.hpp"
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
	// candidate
	llvm::raw_os_ostream los(os);
	// input
	for (const auto &iter : InputRegValueMap_) {
		los << std::string(indent, ' ') << "input " << iter.first << " : ";
		iter.second->print(los);
		los << '\n';
	}
	// output
	for (const auto &iter : OutputRegValueMap_) {
		los << std::string(indent, ' ') << "output " << iter.first << " : ";
		iter.second->print(los);
		los << '\n';
	}
	los.flush();
}

// Build (Find) CCA Pattern in the Codes with a Pattern Graph Instance
CCAPattern *CCAPattern::get(CCAPatternGraph *Graph,
							const std::vector<Instruction *> &Candidate,
							std::set<Instruction *> &Removed,
							const std::set<Instruction *> &UnRemovable) {

	unsigned length = Candidate.size();
	auto reginfo = Graph->output_reginfo();
	if (length != reginfo.size()) return nullptr;

	CCAPattern *P = new CCAPattern(Candidate);
	for (unsigned idx = 0; idx < length; ++idx) P->OutputRegValueMap_.insert({reginfo.at(idx).second, Candidate.at(idx)});
	if (!Graph->matchWithCode(Candidate, UnRemovable, Removed, P->InputRegValueMap_, P->OutputRegValueMap_)) {
		delete P;
		P = nullptr;
	}
	return P;
}

// Build CCA Instruction from Matched Patterns
void CCAPattern::build(unsigned int ccaid, LLVMContext &Context) {
	Type *VoidTy = Type::getVoidTy(Context);
	Type *Int32Ty = Type::getInt32Ty(Context);

	// Prepare CCA Inline Assembly
	unsigned CCAInputMoveLength = InputRegValueMap_.size();
	FunctionType *CCAInputMoveInstFT = FunctionType::get(VoidTy, std::vector<Type *>(CCAInputMoveLength, Int32Ty), false);
	std::string CCAInputMoveAsmStr = "#removethiscomment cca_move $0";
	std::string CCAInputMoveConstraints = "r";
	for (unsigned i = 1; i < CCAInputMoveLength; ++i) {
		CCAInputMoveAsmStr += (", $" + std::to_string(i));
		CCAInputMoveConstraints += ",r";
	}
	InlineAsm *CCAInputMoveIA = InlineAsm::get(CCAInputMoveInstFT, CCAInputMoveAsmStr, CCAInputMoveConstraints, true);

	FunctionType *CCAInstFT = FunctionType::get(VoidTy, false);
	InlineAsm *CCAInstIA = InlineAsm::get(CCAInstFT, "#removethiscomment cca " + std::to_string(ccaid), "", true);

#if 0
	unsigned CCAOutputMoveLength = OutputRegValueMap_.size();
	FunctionType *CCAOutputMoveInstFT = FunctionType::get(
		CCAOutputMoveLength == 1 ? Int32Ty : StructType::get(Context, std::vector<Type *>(CCAOutputMoveLength, Int32Ty)), VoidTy, false);
	std::string CCAOutputMoveAsmStr = "#removethiscomment cca_moveout $0";
	std::string CCAOutputMoveConstraints = "=r";
	for (unsigned i = 1; i < CCAOutputMoveLength; ++i) {
		CCAOutputMoveAsmStr += (", $" + std::to_string(i));
		CCAOutputMoveConstraints += ",=r";
	}
	InlineAsm *CCAOutputMoveIA = InlineAsm::get(CCAOutputMoveInstFT, CCAOutputMoveAsmStr, CCAOutputMoveConstraints, true);
#else
	FunctionType *CCAOutputMoveInstFT = FunctionType::get(Int32Ty, VoidTy, false);
	std::string CCAOutputMoveAsmStr = "#removethiscomment move $0, r" + std::to_string(OutputRegValueMap_.begin()->first);
	std::string CCAOutputMoveConstraints = "=r";
	InlineAsm *CCAOutputMoveIA = InlineAsm::get(CCAOutputMoveInstFT, CCAOutputMoveAsmStr, CCAOutputMoveConstraints, true);
#endif

	// Build Instructions
	// Move Input Value to Register
	std::vector<Value *> CCAInputMoveOperands;
	for (unsigned i = 0; i < CCAInputMoveLength; ++i) CCAInputMoveOperands.push_back(InputRegValueMap_.at(24 + i));
	CallInst *CCAInputMoveInst = CallInst::Create(FunctionCallee(CCAInputMoveInstFT, CCAInputMoveIA), CCAInputMoveOperands);
	CCAInputMoveInst->setTailCall(true);
	// Run CCA
	CallInst *CCACallInst = CallInst::Create(FunctionCallee(CCAInstFT, CCAInstIA));
	CCACallInst->setTailCall(true);
	// Move Output Value to Register
	std::vector<Instruction *> CCAOutputMoveInstVec;
	CallInst *CCAOutputMoveInst = CallInst::Create(FunctionCallee(CCAOutputMoveInstFT, CCAOutputMoveIA), "ccamoveout");
	CCAOutputMoveInst->setTailCall(false);
	CCAOutputMoveInstVec.push_back(CCAOutputMoveInst);
#if 0
	if (CCAOutputMoveLength == 1) CCAOutputInst_.push_back(CCAOutputMoveInst);
	else {
		for (unsigned i = 0; i < CCAOutputMoveLength; ++i) {
			Instruction *I = ExtractValueInst::Create(CCAOutputMoveInst, {i}, "extractccaout");
			CCAOutputMoveInstVec.push_back(I);
			CCAOutputInst_.push_back(I);
		}
	}
#else
	CCAOutputInst_.push_back(CCAOutputMoveInst);
#endif

	// Insert Instructions & Replace All Uses
	Instruction *InsertPosFromUse = nullptr, *InsertPos = nullptr;
	for (auto mapIter : OutputRegValueMap_) {
		if (!isa<Instruction>(mapIter.second)) continue;
		Instruction *I = cast<Instruction>(mapIter.second);
		if (InsertPosFromUse == nullptr) InsertPosFromUse = I;
		else if (I->comesBefore(InsertPosFromUse))
			InsertPosFromUse = I;
	}
	/*
	for (auto mapIter : InputRegValueMap_) {
		if(!isa<Instruction>(mapIter.second)) continue;
		Instruction *I = cast<Instruction>(mapIter.second);
		if (InsertPosFromOperand == nullptr) InsertPosFromOperand = I;
		else if (InsertPosFromOperand->comesBefore(I)) InsertPosFromOperand = I;
	}
	if(!InsertPosFromOperand->comesBefore(InsertPosFromUse)) ;
	*/
	InsertPos = InsertPosFromUse;

	CCAInputMoveInst->insertBefore(InsertPos);
	CCACallInst->insertBefore(InsertPos);
	for (auto *I : CCAOutputMoveInstVec) I->insertBefore(InsertPos);

	/*
	for (auto mapIter : InputRegValueMap_) {
		if(!isa<Instruction>(mapIter.second)) continue;
		Instruction *I = cast<Instruction>(mapIter.second);
		if(CCAInputMoveInst->comesBefore(I)) I->moveBefore(CCAInputMoveInst);
	}
	*/
}

void CCAPattern::resolve(void) {
	for (unsigned i = 0; i < CCAOutputInst_.size(); ++i) OutputRegValueMap_.at(24 + i)->replaceAllUsesWith(CCAOutputInst_.at(i));
}

//--------------------------------------------
// Candidate Iterator for Universal Pass
//--------------------------------------------

class CandidateInstIter {
  private:
	const unsigned op_;
	BasicBlock::iterator begin_, cur_, end_;

  public:
	CandidateInstIter(unsigned op, BasicBlock::iterator begin, BasicBlock::iterator end) : op_(op), begin_(begin), cur_(begin), end_(end) {
		if (cur_->getOpcode() != op_) increase();
	}

	void increase(void) {
		while (cur_ != end_) {
			cur_++;
			if (cur_->getOpcode() == op_) break;
		}
	}

	void reset(void) { cur_ = begin_; }
	bool valid(void) const { return cur_ != end_; }
	unsigned size(void) const {
		unsigned numInst = 0;
		for (auto it = begin_; it != end_; ++it)
			if (cur_->getOpcode() == op_) numInst++;
		return numInst;
	}
	BasicBlock::iterator get(void) const { return cur_; }

	bool operator==(const CandidateInstIter &Iter) const {
		if (this->cur_ == Iter.cur_) return true;
		else
			return false;
	}
};

class CandidateIter {
  private:
	bool terminated_;
	std::vector<CandidateInstIter> I_;

  public:
	CandidateIter(const std::vector<unsigned> &opcode, BasicBlock::iterator begin, BasicBlock::iterator end) : terminated_(false) {
		for (const auto &op : opcode) { I_.push_back(CandidateInstIter(op, begin, end)); }
		while (valid() && duplicated()) increase();
	}

	void increase(void) {
		bool t = true;
		for (auto &Iter : I_) {
			Iter.increase();
			if (Iter.valid()) {
				t = false;
				break;
			}
			Iter.reset();
		}
		if (t) terminated_ = true;
	}

	bool valid(void) const {
		if (terminated_) return false;
		for (auto &Iter : I_)
			if (!Iter.valid()) return false;
		return true;
	}

	bool duplicated(void) const {
		for (unsigned i = 0; i < I_.size(); ++i) {
			for (unsigned j = 0; j < I_.size(); ++j) {
				if (i != j && I_.at(i) == I_.at(j)) return true;
			}
		}
		return false;
	}

	unsigned size(void) const {
		unsigned size = 1;
		for (const auto &Iter : I_) size *= Iter.size();
		return size;
	}

	bool isInSet(const std::set<Instruction *> Set) const {
		for (auto Iter : I_)
			if (Set.find(cast<Instruction>(Iter.get())) != Set.end()) return true;
		return false;
	}

	std::vector<Instruction *> get(void) {
		std::vector<Instruction *> ret;
		for (const auto &Iter : I_) ret.push_back(cast<Instruction>(Iter.get()));
		return ret;
	}
};

//--------------------------------------------
// CCA Universal Pass
//--------------------------------------------
// Constructor
CCAUniversalPass::CCAUniversalPass(std::string patternStr) : patternStr_(patternStr) {
	/*
	// Parse Input String
	std::vector<std::string> tokenVec;
	std::stringstream ss(patternStr);
	std::string line;
	while (getline(ss, line)) {
		size_t prev = 0, pos;
		while ((pos = line.find_first_of(" =+/-*:();", prev)) != std::string::npos) {
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
	std::vector<CCAPatternSubGraph *> SubGraphs;
	while (pos < tokenVec.size()) {
		// Find Semicolon
		while (++pos < tokenVec.size() && tokenVec.at(pos) != ";")
			;
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

		std::cout << std::endl;
		SubGraphs.push_back(new CCAPatternSubGraph(oregtype, oregnum, std::vector<std::string>(tokenVec.begin() + prev + 3, tokenVec.begin() + pos)));
		// Update prev
		prev = pos;
	}
	G_ = new CCAPatternGraph(SubGraphs);
	*/
	G_ = parser::parsePatternStr(patternStr);
	// Verbose
	outs() << "[PIM-CCA-PASS] Build Pattern Graph using\"" << patternStr << "\"\n";
	G_->print(2, outs());
}

// Pass Run
PreservedAnalyses CCAUniversalPass::run(Function &F, FunctionAnalysisManager &) {
	std::set<Instruction *> RemovedInsts;
	std::set<Instruction *> ReplacedInsts;
	std::vector<CCAPattern *> PatternVec;

	outs() << "[PIM-CCA-PASS] Start Pattern Search in Function [" << F.getName() << "] for pattern = \"" << patternStr_ << "\"\n";
	outs().flush();
	for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
		CandidateIter CIter(G_->opcode(), FuncIter->begin(), FuncIter->end());

		// unsigned iter = 0, size = CIter.size();
		while (CIter.valid()) {
			// Get Patterns using Candidates
			std::vector<Instruction *> Candidate = CIter.get();
			/*
			if (iter % 1000 == 0) {
				outs() << "  CANDIDATE [" << iter << " / " << size << "] (valid = " << (CIter.valid() ? 'T' : 'F') << "): \n";
				for (auto *I : Candidate) {
					outs() << std::string(4, ' ');
					I->print(outs());
					outs() << '\n';
				}
			}
			*/
			CCAPattern *P = CCAPattern::get(G_, Candidate, RemovedInsts, ReplacedInsts);
			if (P != nullptr) {
				PatternVec.push_back(P);
				ReplacedInsts.insert(Candidate.begin(), Candidate.end());
				auto ORVM = P->ORVM();
				for (auto mapIter : ORVM) ReplacedInsts.insert(cast<Instruction>(mapIter.second));
			}
			// Update Iterators
			CIter.increase();
			while (CIter.valid() && (CIter.duplicated() || CIter.isInSet(RemovedInsts) || CIter.isInSet(ReplacedInsts))) CIter.increase();
			// ++iter;
		}
	}

	// Verbose
	if (!PatternVec.empty()) {
		outs() << "[PIM-CCA-PASS] Found Patterns in Function [" << F.getName() << "], pattern = \"" << patternStr_ << "\"\n";
		outs().flush();
		outs() << "  - removed: \n";
		for (const auto &iter : RemovedInsts) {
			outs() << std::string(4, ' ');
			iter->print(outs());
			outs() << '\n';
		}
		outs().flush();
		outs() << "  - replaced: \n";
		for (const auto &iter : ReplacedInsts) {
			outs() << std::string(4, ' ');
			iter->print(outs());
			outs() << '\n';
		}
		outs().flush();
	}

	// Build CCA Instructions from Patterns
	for (auto &P : PatternVec) P->build(G_->rule_number(), F.getContext());
	for (auto &P : PatternVec) P->resolve();

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
		outs() << "[PIM-CCA-PASS][ERROR] Cannot Resolve All the Intermediate Instructions\n";
		outs().flush();
		for (const auto &I : RemovedInsts) {
			I->print(outs());
			outs() << '\n';
			for (const auto &V : I->users()) {
				outs() << "  - ";
				V->print(outs());
				outs() << '\n';
			}
		}
	}

	// Reorder Instructions
	for (Function::iterator FuncIter = F.begin(); FuncIter != F.end(); ++FuncIter) {
		bool changed = true;
		while (changed) {
			changed = false;
			for (BasicBlock::iterator BBIter = FuncIter->begin(); BBIter != FuncIter->end(); ++BBIter) {
				Instruction *I = cast<Instruction>(BBIter);
				if (isa<PHINode>(I)) continue;
				for (unsigned idx = 0; idx < I->getNumOperands(); ++idx) {
					Value *V = I->getOperand(idx);
					if (!isa<Instruction>(V)) continue;
					if (isa<PHINode>(V)) continue;
					Instruction *OPI = cast<Instruction>(V);
					if (OPI->getParent() == I->getParent() && !OPI->comesBefore(I)) {
						/*
						outs() << "[PASS] Fix Instruction Orders\n";
						I->print(outs());
						outs() << '\n';
						OPI->print(outs());
						outs() << '\n';
						*/
						OPI->moveBefore(I);
						changed = true;
					}
				}
				if (changed) break;
			}
		}
	}

	// F.print(outs());

	return PreservedAnalyses::all();
}

} // namespace cca
} // namespace llvm
