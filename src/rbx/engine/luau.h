//
// Created by savage on 17.04.2025.
//

#pragma once

#include <string_view>
#include <cstdint>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <cmath>

#define LUAVM_SHUFFLE_COMMA ,
#define LUAVM_SHUFFLE_OTHER ;

#define LUAVM_SHUFFLE3(sep, a1, a2, a3) a1 sep a3 sep a2
#define LUAVM_SHUFFLE4(sep, a1, a2, a3, a4) a4 sep a3 sep a1 sep a2
#define LUAVM_SHUFFLE5(sep, a1, a2, a3, a4, a5) a5 sep a3 sep a2 sep a1 sep a4
#define LUAVM_SHUFFLE6(sep, a1, a2, a3, a4, a5, a6) a1 sep a3 sep a2 sep a6 sep a5 sep a4
#define LUAVM_SHUFFLE7(sep, a1, a2, a3, a4, a5, a6, a7) a6 sep a2 sep a7 sep a3 sep a1 sep a4 sep a5
#define LUAVM_SHUFFLE8(sep, a1, a2, a3, a4, a5, a6, a7, a8) a1 sep a8 sep a4 sep a6 sep a2 sep a7 sep a5 sep a3
#define LUAVM_SHUFFLE9(sep, a1, a2, a3, a4, a5, a6, a7, a8, a9) a9 sep a7 sep a3 sep a5 sep a1 sep a6 sep a2 sep a8 sep a4


namespace VMValues {
    /*
                  SET                                 GET
        VMValue0: Data = Value                     || Value = Data
        VMValue1: Data = (Value + (Data + Offset)) || Value = (Data - (Value + Offset))
        VMValue2: Data = ((Data + Offset) - Value) || Value = ((v + Offset) - Data)
        VMValue3: Data = (Value ^ (Data + Offset)) || Value = ((v + Offset) ^ Data)
        VMValue4: Data = (Value - (Data + Offset)) || Value = ((v + Offset) + Data)
        NOTE: Data = *(deref_type*)(v + Offset)

        NOTE: a2 + = SET
    */

    template <typename T>
    struct VMValue1
    {
    public:
        VMValue1() = default;  // Add default constructor

        operator const T() const
        {
            return (T)((uintptr_t)storage - (uintptr_t)this);
        }

        VMValue1(const VMValue1& other)
        {
            *this = (T)other;
        }

        VMValue1& operator=(const VMValue1& other)
        {
            *this = (T)other;
            return *this;
        }

        VMValue1& operator=(const T& value)
        {
            storage = (T)((uintptr_t)value + (uintptr_t)this);
            return *this;
        }

        const T operator->() const
        {
            return operator const T();
        }
    private:
        T storage{};
    };

    template <typename T>
    struct VMValue2
    {
    public:
        VMValue2() = default;

        operator const T() const
        {
            return (T)((uintptr_t)this - (uintptr_t)storage);
        }

        VMValue2(const VMValue2& other) { *this = (T)other; }

        VMValue2& operator=(const VMValue2& other)
        {
            *this = (T)other;
            return *this;
        }

        VMValue2& operator=(const T& value)
        {
            storage = (T)((uintptr_t)this - (uintptr_t)value);
            return *this;
        }

        const T operator->() const { return operator const T(); }
    private:
        T storage{};
    };

    template <typename T>
    struct VMValue3
    {
    public:
        VMValue3() = default;

        operator const T() const
        {
            return (T)((uintptr_t)storage ^ (uintptr_t)this);
        }

        VMValue3(const VMValue3& other) { *this = (T)other; }

        VMValue3& operator=(const VMValue3& other)
        {
            *this = (T)other;
            return *this;
        }

        VMValue3& operator=(const T& value)
        {
            storage = (T)((uintptr_t)value ^ (uintptr_t)this);
            return *this;
        }

        const T operator->() const { return operator const T(); }
    private:
        T storage{};
    };

    template <typename T>
    struct VMValue4
    {
    public:
        VMValue4() = default;

        operator const T() const
        {
            return (T)((uintptr_t)this + (uintptr_t)storage);
        }

        VMValue4(const VMValue4& other) { *this = (T)other; }

        VMValue4& operator=(const VMValue4& other)
        {
            *this = (T)other;
            return *this;
        }

        VMValue4& operator=(const T& value)
        {
            storage = (T)((uintptr_t)value - (uintptr_t)this);
            return *this;
        }

        const T operator->() const { return operator const T(); }
    private:
        T storage{};
    };
}

// VMValue1 = sub_add
// VMValue2 = sub_sub
// VMValue3 = xor_xor
// VMValue4 = add_add

#define sub_add VMValues::VMValue1
#define sub_sub VMValues::VMValue2
#define xor_xor VMValues::VMValue3
#define add_sub VMValues::VMValue4

#define encryption_group_1 add_sub
#define encryption_group_2 sub_add
#define encryption_group_3 sub_sub
#define encryption_group_4 xor_xor

//#define c_func_enc encryption_group_1

#define ts_hash_enc VMValues::VMValue3
#define p_debugname_enc VMValues::VMValue3

#define p_member2_enc VMValues::VMValue2
#define c_debugname_enc VMValues::VMValue2

#define p_debuginsn_enc VMValues::VMValue4
#define c_cont_enc VMValues::VMValue4
#define u_meta_enc VMValues::VMValue4

#define p_typeinfo_enc VMValues::VMValue1
#define l_stacksize_enc VMValues::VMValue1