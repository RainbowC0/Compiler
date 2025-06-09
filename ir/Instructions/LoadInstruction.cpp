#include "LoadInstruction.h"
#include "VoidType.h"

LoadInstruction::LoadInstruction(Function * _func, Value * addr, Type * type)
    : Instruction(_func, IRINST_OP_LOAD, type)
{
    addOperand(addr);
}

void LoadInstruction::toString(std::string &str) {
    Value *ptr = getOperand(0);
    str = getIRName() + " = load ptr "+ptr->getIRName()+", align 4";
}
