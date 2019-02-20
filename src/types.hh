#pragma once

#include <llvm/IR/Type.h>

namespace ast {
    enum type {
        t_bool,
        u8, u16, u32, u64,
        i8, i16, i32, i64,
        f16, f32, f64,
    };
    static bool type_is_bool(type t) {
        return t == t_bool;
    }
    static bool type_is_integer(type t) {
        return t >= u8 && t <= i64;
    }
    static bool type_is_signed_integer(type t) {
        return t >= i8 && t <= i64;
    }
    static bool type_is_unsigned_integer(type t) {
        return t >= u8 && t <= u64;
    }
    static bool type_is_float(type t) {
        return t >= f16 && t <= f64;
    }
    static bool type_is_number(type t) {
        return t >= u8 && t <= f64;
    }
    static bool llvm_type_is_bool(llvm::Type* t) {
        return t->isIntegerTy(1);
    }
    static bool llvm_type_is_integer(llvm::Type* t) {
        return t->isIntegerTy() && t->getIntegerBitWidth() > 1;
    }
    static bool llvm_type_is_signed_integer(llvm::Type* t) {
        //TODO check sign
        return llvm_type_is_integer(t) && true;
    }
    static bool llvm_type_is_unsigned_integer(llvm::Type* t) {
        //TODO check sign
        return llvm_type_is_integer(t) && true;
    }
    static bool llvm_type_is_float(llvm::Type* t) {
        return t->isFloatingPointTy();
    }
    static bool llvm_type_is_number(llvm::Type* t) {
        return llvm_type_is_integer(t) || llvm_type_is_float(t);
    }
}
