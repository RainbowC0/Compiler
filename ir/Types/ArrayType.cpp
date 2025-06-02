///
/// @file ArrayType.cpp
/// @brief 数组类型描述类实现
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

#include "ArrayType.h"

// ArrayType的方法已在头文件中完全实现
// 由于使用了StorageSet模板类，所有方法都是内联的

[[nodiscard]] bool ArrayType::isMultiDimensional() const
{
    return elementType->isArrayType();
}

[[nodiscard]] uint32_t ArrayType::getDimensions() const
{
    uint32_t i = 1;
    const Type *tp = elementType;
    while (tp->isArrayType()) {
        tp = ((ArrayType*)tp)->elementType;
        i++;
    }
    return i;
}

[[nodiscard]] std::vector<uint32_t> ArrayType::getDimensionSizes() const
{
    std::vector<uint32_t> sizes;
    const Type *tp = this;
    do {
        sizes.push_back(((ArrayType*)tp)->numElements);
        tp = (((ArrayType*)tp)->elementType);
    } while (tp && tp->isArrayType());

    return sizes;
}

[[nodiscard]] const Type* ArrayType::getBaseElementType() const
{
    const ArrayType *tp = this;
    while (tp->elementType && tp->elementType->isArrayType())
        tp = (ArrayType*)tp->elementType;
    return tp->elementType;
}

void ArrayType::setBaseElementType(const Type *type) {
    ArrayType *tp = (ArrayType*)this;
    while (tp->elementType && tp->elementType->isArrayType())
        tp = (ArrayType*)((ArrayType*)tp)->elementType;
    tp->elementType = type;
}

const ArrayType * ArrayType::get(Type * elementType, uint32_t numElements)
{
    static StorageSet<ArrayType, ArrayTypeHasher, ArrayTypeEqual> storageSet;
    return storageSet.get(elementType, numElements);
}

const ArrayType* ArrayType::createMultiDimensional(Type* baseType, const std::vector<uint32_t>& dimensions)
{
    if (dimensions.empty()) {
        return nullptr;
    }
    
    Type* currentType = baseType;
    for (int i = dimensions.size() - 1; i >= 0; --i) {
        currentType = new ArrayType(currentType, dimensions[i]);
    }
    
    return static_cast<const ArrayType*>(currentType);
}
