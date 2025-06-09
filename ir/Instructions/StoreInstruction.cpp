#include "StoreInstruction.h"
#include "VoidType.h"

StoreInstruction::StoreInstruction(Function * _func, Value * addr, Value * srcVal1)
    : Instruction(_func, IRINST_OP_STORE, VoidType::getType())
{
    addOperand(addr);
    addOperand(srcVal1);
}

void StoreInstruction::toString(std::string &str) {
    Value *ptr = getOperand(0), *src = getOperand(1);
    str = "store "+src->getIRName()+", ptr "+ptr->getIRName()+", align 4";
}
