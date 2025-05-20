///
/// @file TypeSystem.h
/// @brief 类型系统辅助类，包括类型转换等功能
///
/// @author [用户]
/// @version 1.0
/// @date [当前日期]
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>[当前日期] <td>1.0     <td>[用户]  <td>新建
/// </table>
///

#pragma once

#include "Type.h"
#include "Types/IntegerType.h"
#include "Types/FloatType.h"
#include "Types/ArrayType.h"

class TypeSystem {
public:
    ///
    /// @brief 获取两个类型的公共类型（用于隐式类型转换）
    /// @param type1 第一个类型
    /// @param type2 第二个类型
    /// @return 公共类型指针
    ///
    static Type * getCommonType(const Type * type1, const Type * type2)
    {
        // 如果是相同类型，直接返回
        if (type1 == type2) {
            return const_cast<Type *>(type1);
        }

        // 如果有一个是浮点类型，结果就是浮点类型
        if (type1->isFloatType() || type2->isFloatType()) {
            return FloatType::getTypeFloat();
        }

        // 如果都是整数类型
        if (type1->isIntegerType() && type2->isIntegerType()) {
            // 都是整数类型，返回较大的整数类型
            const IntegerType * intType1 = static_cast<const IntegerType *>(type1);
            const IntegerType * intType2 = static_cast<const IntegerType *>(type2);

            if (intType1->getBitWidth() >= intType2->getBitWidth()) {
                return const_cast<Type *>(type1);
            } else {
                return const_cast<Type *>(type2);
            }
        }

        // 其他情况，暂时不支持
        return const_cast<Type *>(type1);
    }

    ///
    /// @brief 检查类型是否兼容（是否可以进行隐式转换）
    /// @param fromType 源类型
    /// @param toType 目标类型
    /// @return 是否兼容
    ///
    static bool isCompatible(const Type * fromType, const Type * toType)
    {
        // 相同类型一定兼容
        if (fromType == toType) {
            return true;
        }

        // 整数类型可以转换为浮点类型
        if (fromType->isIntegerType() && toType->isFloatType()) {
            return true;
        }

        // 布尔类型可以转换为整数
        if (fromType->isInt1Byte() && toType->isInt32Type()) {
            return true;
        }

        // 浮点类型不能隐式转换为整数类型
        if (fromType->isFloatType() && toType->isIntegerType()) {
            return false;
        }

        // 数组类型的特殊处理
        if (fromType->isArrayType() && toType->isArrayType()) {
            const ArrayType * arrayFrom = static_cast<const ArrayType *>(fromType);
            const ArrayType * arrayTo = static_cast<const ArrayType *>(toType);

            // 元素类型必须兼容
            const Type * fromElementType = arrayFrom->getElementType();
            const Type * toElementType = arrayTo->getElementType();
            return isCompatible(fromElementType, toElementType);
        }

        // 其他类型暂不支持隐式转换
        return false;
    }

    ///
    /// @brief 根据操作符和操作数类型获取结果类型
    /// @param op 操作符
    /// @param type1 第一个操作数类型
    /// @param type2 第二个操作数类型
    /// @return 结果类型
    ///
    static Type * getBinaryResultType(IRInstOperator op, const Type * type1, const Type * type2)
    {
        // 获取操作数的公共类型
        Type * commonType = getCommonType(type1, type2);

        // 关系运算符结果是布尔类型
        switch (op) {
            case IROP(IEQ):
            case IROP(INE):
            case IROP(IGT):
            case IROP(ILT):
            case IROP(IGE):
            case IROP(ILE):
            case IROP(FEQ):
            case IROP(FNE):
            case IROP(FGT):
            case IROP(FLT):
            case IROP(FGE):
            case IROP(FLE):
                return IntegerType::getTypeBool();

            default:
                // 加减乘除等运算符结果是操作数的公共类型
                return commonType;
        }
    }

    ///
    /// @brief 获取合适的操作符（基于操作数类型）
    /// @param baseOp 基础操作符
    /// @param type1 第一个操作数类型
    /// @param type2 第二个操作数类型
    /// @return 适合的操作符
    ///
    static IRInstOperator getAppropriateOp(IRInstOperator baseOp, const Type * type1, const Type * type2)
    {
        // 获取操作数的公共类型
        Type * commonType = getCommonType(type1, type2);

        // 根据公共类型选择合适的操作符
        if (commonType->isFloatType()) {
            // 如果公共类型是浮点型，使用浮点操作符
            switch (baseOp) {
                case IROP(IADD):
                    return IROP(FADD);
                case IROP(ISUB):
                    return IROP(FSUB);
                case IROP(IMUL):
                    return IROP(FMUL);
                case IROP(IDIV):
                    return IROP(FDIV);
                case IROP(IMOD):
                    return IROP(FMOD);
                case IROP(IEQ):
                    return IROP(FEQ);
                case IROP(INE):
                    return IROP(FNE);
                case IROP(IGT):
                    return IROP(FGT);
                case IROP(IGE):
                    return IROP(FGE);
                case IROP(ILT):
                    return IROP(FLT);
                case IROP(ILE):
                    return IROP(FLE);
                default:
                    return baseOp;
            }
        } else {
            // 否则使用整数操作符
            return baseOp;
        }
    }
};