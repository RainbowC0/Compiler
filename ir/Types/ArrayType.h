///
/// @file ArrayType.h
/// @brief 数组类型描述类
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
#include "StorageSet.h"
#include <cstdint>
#include <vector>

///
/// @brief 数组类型
///
class ArrayType : public Type {
    ///
    /// @brief Hash用结构体，提供Hash函数
    ///
    struct ArrayTypeHasher final {
        size_t operator()(const ArrayType & type) const noexcept
        {
            return std::hash<const Type *>{}(type.getElementType()) ^ std::hash<uint32_t>{}(type.getNumElements());
        }
    };

    ///
    /// @brief 判断两者相等的结构体，提供等于函数
    ///
    struct ArrayTypeEqual final {
        size_t operator()(const ArrayType & lhs, const ArrayType & rhs) const noexcept
        {
            return lhs.getElementType() == rhs.getElementType() && lhs.getNumElements() == rhs.getNumElements();
        }
    };

public:
    /// @brief ArrayType的构造函数
    /// @param[in] elementType 数组元素的类型
    /// @param[in] numElements 数组的元素数量
    explicit ArrayType(const Type * elementType, uint32_t numElements)
        : Type(ArrayTyID), elementType(elementType), numElements(numElements)
    {}

    ///
    /// @brief 返回数组元素类型
    /// @return const Type*
    ///
    [[nodiscard]] const Type * getElementType() const
    {
        return elementType;
    }

    ///
    /// @brief 返回数组元素数量
    /// @return uint32_t
    ///
    [[nodiscard]] uint32_t getNumElements() const
    {
        return numElements;
    }

    ///
    /// @brief 获取数组类型
    /// @param elementType 元素类型
    /// @param numElements 元素数量
    /// @return const ArrayType*
    ///
    static const ArrayType * get(Type * elementType, uint32_t numElements);

    ///
    /// @brief 获取类型的IR标识符
    /// @return std::string IR标识符
    ///
    [[nodiscard]] std::string toString() const override
    {
        return "[" + std::to_string(numElements) + " x " + elementType->toString() + "]";
    }

    ///
    /// @brief 获得类型所占内存空间大小
    /// @return int32_t
    ///
    [[nodiscard]] int32_t getSize() const override
    {
        return elementType->getSize() * numElements;
    }

    ///
    /// @brief 检查是否是多维数组
    /// @return true 是多维数组 false 不是
    ///
    [[nodiscard]] bool isMultiDimensional() const;

    ///
    /// @brief 获取数组的维度数
    /// @return 维度数
    ///
    [[nodiscard]] uint32_t getDimensions() const;

    ///
    /// @brief 获取多维数组各维度大小
    /// @return 维度大小数组
    ///
    [[nodiscard]] std::vector<uint32_t> getDimensionSizes() const;

    ///
    /// @brief 获取数组最内层元素类型（非数组类型）
    /// @return 最内层元素类型
    ///
    [[nodiscard]] const Type* getBaseElementType() const;

    void setBaseElementType(const Type*);
    ///
    /// @brief 从多维索引创建多维数组类型
    /// @param baseType 基础元素类型
    /// @param dimensions 各维度大小
    /// @return 多维数组类型
    ///
    static const ArrayType* createMultiDimensional(Type* baseType, const std::vector<uint32_t>& dimensions);

private:
    ///
    /// @brief 数组元素类型
    ///
    const Type * elementType = nullptr;

    ///
    /// @brief 数组元素数量
    ///
    uint32_t numElements = 0;
};
