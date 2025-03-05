#include "Instrumentation/Utils.hpp"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

namespace llvm {
namespace cca {
// Get Binary Operator from Operands
User::op_iterator GetBinaryOperatorInOperands(User *U, Instruction::BinaryOps Op) {
	U->operands();
	for (User::op_iterator UseIter = U->op_begin(); UseIter != U->op_end(); ++UseIter) {
		if (isa<BinaryOperator>(UseIter) && cast<BinaryOperator>(UseIter)->getOpcode() == Op) return UseIter;
	}
	return U->op_end();
}
// Get Binary Operator in Operands of Binary Operator
BinaryOperator *GetBinaryOperatorInOperandsInBinaryOperator(BinaryOperator *Inst, Instruction::BinaryOps Op, Value *&NonBOOperand) {
	if (isa<BinaryOperator>(Inst->getOperand(0)) && cast<BinaryOperator>(Inst->getOperand(0))->getOpcode() == Op) {
		NonBOOperand = Inst->getOperand(1);
		return cast<BinaryOperator>(Inst->getOperand(0));
	} else if (isa<BinaryOperator>(Inst->getOperand(1)) && cast<BinaryOperator>(Inst->getOperand(1))->getOpcode() == Op) {
		NonBOOperand = Inst->getOperand(0);
		return cast<BinaryOperator>(Inst->getOperand(1));
	}
	return nullptr;
}
// Check Other Use Exist except Specific User
bool CheckOtherUseExist(Value *V, User *U) {
	bool OtherUseExist = false;
	for (auto UserIter : V->users()) {
		if (UserIter != U) {
			OtherUseExist = true;
			break;
		}
	}
	return OtherUseExist;
}

// Check Other Use Exist except Specific User and Store Instruction
bool CheckOtherUseExistWithoutStore(Value *V, User *U, std::vector<StoreInst *> &SVec) {
	bool OtherUseExist = false;
	for (auto UserIter : V->users()) {
		if (UserIter == U) continue;
		if (isa<StoreInst>(UserIter)) {
			SVec.push_back(cast<StoreInst>(UserIter));
			continue;
		}
		OtherUseExist = true;
		break;
	}
	return OtherUseExist;
}

} // namespace cca
} // namespace llvm
