#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

#include <sstream>

namespace ast {
    enum primitive_type {
        t_void,
        t_bool,
        u8, u16, u32, u64,
        i8, i16, i32, i64,
        f16, f32, f64,
    };
    using type = primitive_type;
    using identifier = size_t;
    struct struct_type;
    struct array_type;
    using constructed_type = std::variant<primitive_type, std::unique_ptr<struct_type>, std::unique_ptr<array_type>>;
    struct struct_type {
        std::vector<std::pair<ast::identifier, ast::constructed_type>> fields;
    };
    struct array_type {
        ast::constructed_type element_type;
        size_t length;
    };
    static std::string type_to_string(type t) {
        struct type_visitor {
            std::ostringstream s;
            void operator()(primitive_type& primitive_type) {
                switch (primitive_type) {
                    case t_void:    { s << "void";  break; }
                    case t_bool:    { s << "bool";  break; }
                    case u8:        { s << "u8";    break; }
                    case u16:       { s << "u16";   break; }
                    case u32:       { s << "u32";   break; }
                    case u64:       { s << "u64";   break; }
                    case i8:        { s << "i8";    break; }
                    case i16:       { s << "i16";   break; }
                    case i32:       { s << "i32";   break; }
                    case i64:       { s << "i64";   break; }
                    case f16:       { s << "f16";   break; }
                    case f32:       { s << "f32";   break; }
                    case f64:       { s << "f64";   break; }
                    default:        assert(false);
                }
            }
            /*
            void operator()(struct_type& struct_type) {
                s << "struct { ";
                for (auto& field: struct_type.fields) {
                    std::invoke(*this, field);
                    s << " ";
                }
                s << "}";
            }
            void operator()(array_type& array_type) {
                s << "array [ ";
                std::invoke(*this, array_type.element_type);
                s << " * " << array_type.length << " ]";
            }
            */
        };
        type_visitor context;
        std::invoke(context, t);
        return context.s.str();
    }
    static bool type_is_primitive(type t) {
        //TODO can std::variant do this in a neater way?
        struct type_visitor {
            bool operator()(primitive_type& primitive_type) {
                return true;
            }
            bool operator()(struct_type& struct_type) {
                return false;
            }
            bool operator()(array_type& array_type) {
                return false;
            }
        };
        return std::invoke(type_visitor{}, t);
    }
    static bool type_is_bool(primitive_type t) {
        return t == t_bool;
    }
    static bool type_is_integer(primitive_type t) {
        return t >= u8 && t <= i64;
    }
    static bool type_is_signed_integer(primitive_type t) {
        return t >= i8 && t <= i64;
    }
    static bool type_is_unsigned_integer(primitive_type t) {
        return t >= u8 && t <= u64;
    }
    static bool type_is_float(primitive_type t) {
        return t >= f16 && t <= f64;
    }
    static bool type_is_number(primitive_type t) {
        return t >= u8 && t <= f64;
    }
    static bool llvm_type_is_bool(llvm::Type* t) {
        return t->isIntegerTy(1);
    }
    static bool llvm_type_is_integer(llvm::Type* t) {
        return t->isIntegerTy() && t->getIntegerBitWidth() > 1;
    }
    static bool llvm_type_is_float(llvm::Type* t) {
        return t->isFloatingPointTy();
    }
    static bool llvm_type_is_number(llvm::Type* t) {
        return llvm_type_is_integer(t) || llvm_type_is_float(t);
    }

    static llvm::Type* type_to_llvm_type(llvm::LLVMContext &context, primitive_type t) {
        //TODO update this to construct aggregate types in LLVM
        switch (t) {
            case t_void:
                return llvm::Type::getVoidTy(context);
            case t_bool:
                return llvm::Type::getInt1Ty(context);
            case u8:
                return llvm::Type::getInt8Ty(context);
            case u16:
                return llvm::Type::getInt16Ty(context);
            case u32:
                return llvm::Type::getInt32Ty(context);
            case u64:
                return llvm::Type::getInt64Ty(context);
            case i8:
                return llvm::Type::getInt8Ty(context);
            case i16:
                return llvm::Type::getInt16Ty(context);
            case i32:
                return llvm::Type::getInt32Ty(context);
            case i64:
                return llvm::Type::getInt64Ty(context);
            case f16:
                return llvm::Type::getHalfTy(context);
            case f32:
                return llvm::Type::getFloatTy(context);
            case f64:
                return llvm::Type::getDoubleTy(context);
        }
        assert(false);
    }
}
