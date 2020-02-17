#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

#include <sstream>
#include <string>

#include "registry.hh"

namespace ast {
    struct primitive_type {
        enum e : size_t {
            t_void,
            t_bool,
            u8, u16, u32, u64,
            i8, i16, i32, i64,
            f16, f32, f64,
        };
        e value;
        operator e() const { return value; }
        explicit operator bool() = delete;
        constexpr bool operator==(const primitive_type a) const { return value == a.value; }
        constexpr bool operator!=(const primitive_type a) const { return value != a.value; }
        std::string to_string() {
            switch (value) {
                case t_void:    { return "void"; }
                case t_bool:    { return "bool"; }
                case u8:        { return "u8";  }
                case u16:       { return "u16"; }
                case u32:       { return "u32"; }
                case u64:       { return "u64"; }
                case i8:        { return "i8";  }
                case i16:       { return "i16"; }
                case i32:       { return "i32"; }
                case i64:       { return "i64"; }
                case f16:       { return "f16"; }
                case f32:       { return "f32"; }
                case f64:       { return "f64"; }
                default: assert(false);
            }
        }
        llvm::Type* to_llvm_type(llvm::LLVMContext &context) {
            switch (value) {
                case t_void: return llvm::Type::getVoidTy(context);
                case t_bool: return llvm::Type::getInt1Ty(context);
                case u8:     return llvm::Type::getInt8Ty(context);
                case u16:    return llvm::Type::getInt16Ty(context);
                case u32:    return llvm::Type::getInt32Ty(context);
                case u64:    return llvm::Type::getInt64Ty(context);
                case i8:     return llvm::Type::getInt8Ty(context);
                case i16:    return llvm::Type::getInt16Ty(context);
                case i32:    return llvm::Type::getInt32Ty(context);
                case i64:    return llvm::Type::getInt64Ty(context);
                case f16:    return llvm::Type::getHalfTy(context);
                case f32:    return llvm::Type::getFloatTy(context);
                case f64:    return llvm::Type::getDoubleTy(context);
                default:     assert(false);
            }
        }
        bool is_void() {
            return value == t_void;
        }
        bool is_bool() {
            return value == t_bool;
        }
        bool is_integer() {
            return value >= u8 && value <= i64;
        }
        bool is_signed_integer() {
            return value >= i8 && value <= i64;
        }
        bool is_unsigned_integer() {
            return value >= u8 && value <= u64;
        }
        bool is_float() {
            return value >= f16 && value <= f64;
        }
        bool is_number() {
            return value >= u8 && value <= f64;
        }
    };
    struct identifier {
        size_t value;
        constexpr bool operator==(const identifier a) const { return value == a.value; }
        constexpr bool operator!=(const identifier a) const { return value != a.value; }
        std::string& to_string(bi_registry<ast::identifier, std::string>& symbols_registry) {
            return symbols_registry.get(*this);
        }
    };
    struct user_type {
        size_t value;
        constexpr bool operator==(const user_type& a) const { return value == a.value; }
        constexpr bool operator!=(const user_type& a) const { return value != a.value; }
        std::string& to_string(bi_registry<ast::identifier, std::string>& symbols_registry) {
            return symbols_registry.get(ast::identifier{value});
        }
        llvm::Type* to_llvm_type(llvm::LLVMContext &context) {
            return nullptr;
        }
        bool is_void() { return false; }
        bool is_bool() { return false; }
        bool is_integer() { return false; }
        bool is_signed_integer() { return false; }
        bool is_unsigned_integer() { return false; }
        bool is_float() { return false; }
        bool is_number() { return false; }
        bool is_primitive() { return false; }
    };
    struct named_type {
        std::variant<primitive_type, user_type> type;
        constexpr bool operator==(const named_type& a) const { return type == a.type; }
        constexpr bool operator!=(const named_type& a) const { return type != a.type; }
        llvm::Type* to_llvm_type(llvm::LLVMContext &context) {
            return std::visit([&context](auto t) {
                return t.to_llvm_type(context);
            }, type);
        }
        std::string to_string(bi_registry<ast::identifier, std::string>& symbols_registry) {
            if (std::holds_alternative<ast::primitive_type>(type)) {
                return std::get<primitive_type>(type).to_string();
            } else {
                return std::get<user_type>(type).to_string(symbols_registry);
            }
        }
        bool is_void() { return is_primitive() && std::get<primitive_type>(type).is_void(); }
        bool is_bool() { return is_primitive() && std::get<primitive_type>(type).is_bool(); }
        bool is_integer() { return is_primitive() && std::get<primitive_type>(type).is_integer(); }
        bool is_signed_integer() { return is_primitive() && std::get<primitive_type>(type).is_signed_integer(); }
        bool is_unsigned_integer() { return is_primitive() && std::get<primitive_type>(type).is_unsigned_integer(); }
        bool is_float() { return is_primitive() && std::get<primitive_type>(type).is_float(); }
        bool is_number() { return is_primitive() && std::get<primitive_type>(type).is_number(); }
        bool is_primitive() { return std::holds_alternative<primitive_type>(type); }
    };
    struct type;
    struct field {
        ast::named_type type;
        ast::identifier identifier;
    };
    using field_list = std::vector<ast::field>;
    struct struct_type {
        ast::field_list fields;
        llvm::Type* to_llvm_type(llvm::LLVMContext &context) {
            std::vector<llvm::Type*> llvm_fields;
            for (auto field: fields) {
                llvm_fields.push_back(field.type.to_llvm_type(context));
            }
            bool packed = false;
            return llvm::StructType::get(context, llvm::ArrayRef<llvm::Type*>{llvm_fields}, packed);
        }
    };
    struct array_type {
        ast::named_type element_type;
        size_t length;
        llvm::Type* to_llvm_type(llvm::LLVMContext &context) {
            llvm::Type* llvm_element_type = element_type.to_llvm_type(context);
            return llvm::ArrayType::get(llvm_element_type, length);
        }
    };
    struct type {
        std::variant<ast::primitive_type, ast::user_type, std::unique_ptr<struct_type>, std::unique_ptr<array_type>> type_;
        constexpr type(named_type n) {
            std::visit([this](auto p) {
                type_ = p;
            }, n.type);
        }
        constexpr type(primitive_type p) { type_ = p; }
        constexpr type(struct_type& s) { type_ = std::make_unique<struct_type>(std::move(s)); }
        constexpr type(array_type& a) { type_ = std::make_unique<array_type>(std::move(a)); }
        constexpr type() {
        }
        constexpr bool operator==(const ast::type& a) const { return type_ == a.type_; }
        constexpr bool operator!=(const ast::type& a) const { return type_ != a.type_; }
        bool is_void() { return is_primitive() && std::get<primitive_type>(type_).is_void(); }
        bool is_bool() { return is_primitive() && std::get<primitive_type>(type_).is_bool(); }
        bool is_integer() { return is_primitive() && std::get<primitive_type>(type_).is_integer(); }
        bool is_signed_integer() { return is_primitive() && std::get<primitive_type>(type_).is_signed_integer(); }
        bool is_unsigned_integer() { return is_primitive() && std::get<primitive_type>(type_).is_unsigned_integer(); }
        bool is_float() { return is_primitive() && std::get<primitive_type>(type_).is_float(); }
        bool is_number() { return is_primitive() && std::get<primitive_type>(type_).is_number(); }
        bool is_primitive() { return std::holds_alternative<primitive_type>(type_); }
        std::string to_string(bi_registry<ast::identifier, std::string>& symbols_registry) {
            struct type_printer_fn {
                bi_registry<ast::identifier, std::string>& symbols_registry;
                std::ostringstream s;
                void operator()(ast::primitive_type primitive_type) {
                    s << primitive_type.to_string();
                }
                void operator()(ast::user_type user_type) {
                    s << user_type.to_string(symbols_registry);
                }
                void operator()(const std::unique_ptr<ast::struct_type>& struct_type) {
                    s << "struct { ";
                    for (auto field: struct_type->fields) {
                        std::visit(*this, field.type.type);
                        s << " ";
                        s << field.identifier.to_string(symbols_registry);
                        s << "; ";
                    }
                    s << "}";
                }
                void operator()(const std::unique_ptr<ast::array_type>& array_type) {
                    s << "[";
                    std::visit(*this, array_type->element_type.type);
                    s << " ";
                    s << array_type->length;
                    s << "]";
                }
            };
            type_printer_fn type_printer_fn{symbols_registry};
            std::visit(type_printer_fn, type_);
            return type_printer_fn.s.str();
        }
        llvm::Type* to_llvm_type(llvm::LLVMContext &context) {
            struct type_codegen_fn {
                llvm::LLVMContext &context;
                llvm::Type* operator()(ast::primitive_type primitive_type) {
                    return primitive_type.to_llvm_type(context);
                }
                llvm::Type* operator()(ast::user_type user_type) {
                    return user_type.to_llvm_type(context);
                }
                llvm::Type* operator()(const std::unique_ptr<ast::struct_type>& struct_type) {
                    return struct_type->to_llvm_type(context);
                }
                llvm::Type* operator()(const std::unique_ptr<ast::array_type>& array_type) {
                    return array_type->to_llvm_type(context);
                }
            };
            return std::visit(type_codegen_fn{context}, type_);
        }
    };
}
