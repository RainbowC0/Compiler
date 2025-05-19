#include "ConstInt.h"

int32_t ConstInt::zeroReg = -1;

void ConstInt::setZeroReg(int32_t reg) {
    zeroReg = reg;
}

int32_t ConstInt::getVal()
{
    return intVal;
}
