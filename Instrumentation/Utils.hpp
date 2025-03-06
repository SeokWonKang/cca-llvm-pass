#ifndef PIMCCALLVMPASS_INSTRUMENTATION_UTILS_HPP_
#define PIMCCALLVMPASS_INSTRUMENTATION_UTILS_HPP_

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include <vector>

namespace llvm {
namespace cca {
// Check Binary Operator
bool isaBO(Value *V, Instruction::BinaryOps Op);
// Get Binary Operator in Operands
User::op_iterator GetBinaryOperatorInOperands(User *U, Instruction::BinaryOps Op);
// Get Binary Operator in Operands of Binary Operator
BinaryOperator *GetBinaryOperatorInOperandsInBinaryOperator(BinaryOperator *Inst, Instruction::BinaryOps Op, Value *&NonBOOperand);
// Check Other Use Exist except Specific User
bool CheckOtherUseExist(Value *V, User *U);
// Check Other Use Exist except Specific User and Store Instruction
bool CheckOtherUseExistWithoutStore(Value *V, User *U, std::vector<StoreInst *> &SVec);

}; // namespace cca
}; // namespace llvm

#endif // PIMCCALLVMPASS_INSTRUMENTATION_UTILS_HPP_
