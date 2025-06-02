#include "StoreInstruction.h"

StoreInstruction::StoreInstruction(Function * _func, PointerType * ptr, Value * srcVal1, )
    : Instruction(_func, IRINST_OP_STORE, VoidType::getType())
{
    addOperand(ptr);
    addOperand(srcVal1);
}

StoreInstruction::toString(std::string &str) {
    Value *ptr = getOperand(0), *src = getOperand(1);
    str = "store "+src->getIRName()+", ptr "+ptr->getIRName()+", align 4";
}
