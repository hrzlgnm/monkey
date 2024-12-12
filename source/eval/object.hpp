#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <chungus.hpp>
#include <code/code.hpp>
#include <compiler/symbol_table.hpp>
#include <fmt/core.h>

#include "environment_fwd.hpp"

// helper type for std::visit
template<typename... T>
struct overloaded : T...
{
    using T::operator()...;
};
template<class... T>
overloaded(T...) -> overloaded<T...>;

struct object;
using object_ptr = object*;

struct object
{
    enum class object_type : std::uint8_t
    {
        integer,
        boolean,
        string,
        null,
        error,
        array,
        hash,
        function,
        compiled_function,
        closure,
        builtin,
    };
    object() = default;
    object(const object&) = delete;
    object(object&&) = delete;
    auto operator=(const object&) -> object& = delete;
    auto operator=(object&&) -> object& = delete;
    virtual ~object() = default;

    [[nodiscard]] auto is(object_type obj_type) const -> bool { return type() == obj_type; }

    template<typename T>
    [[nodiscard]] auto as() -> T*
    {
        return static_cast<T*>(this);
    }

    [[nodiscard]] auto is_error() const -> bool { return type() == object_type::error; }

    [[nodiscard]] virtual auto is_truthy() const -> bool { return true; }

    [[nodiscard]] virtual auto is_hashable() const -> bool { return false; }

    [[nodiscard]] virtual auto type() const -> object_type = 0;
    [[nodiscard]] virtual auto inspect() const -> std::string = 0;
    [[nodiscard]] virtual auto equals_to(const object_ptr& other) const -> bool = 0;
    bool is_return_value {};
};

auto operator<<(std::ostream& ostrm, object::object_type type) -> std::ostream&;

struct hashable_object : object
{
    using hash_key_type = std::variant<int64_t, std::string, bool>;

    [[nodiscard]] auto is_hashable() const -> bool override { return true; }

    [[nodiscard]] virtual auto hash_key() const -> hash_key_type = 0;
};

struct integer_object : hashable_object
{
    explicit integer_object(int64_t val)
        : value {val}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::integer; }

    [[nodiscard]] auto inspect() const -> std::string override { return std::to_string(value); }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<integer_object>()->value == value;
    }

    int64_t value {};
};

struct boolean_object : hashable_object
{
    explicit boolean_object(bool val)
        : value {val}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return value; }

    [[nodiscard]] auto type() const -> object_type override { return object_type::boolean; }

    [[nodiscard]] auto inspect() const -> std::string override { return value ? "true" : "false"; }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<boolean_object>()->value == value;
    }

    bool value {};
};

static boolean_object false_obj {false};
static boolean_object true_obj {true};

inline auto native_bool_to_object(bool val) -> object_ptr
{
    if (val) {
        return &true_obj;
    }
    return &false_obj;
}

struct string_object : hashable_object
{
    explicit string_object(std::string val)
        : value {std::move(val)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::string; }

    [[nodiscard]] auto inspect() const -> std::string override { return value; }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<string_object>()->value == value;
    }

    std::string value;
};

struct null_object : object
{
    null_object() = default;

    [[nodiscard]] auto is_truthy() const -> bool override { return false; }

    [[nodiscard]] auto type() const -> object_type override { return object_type::null; }

    [[nodiscard]] auto inspect() const -> std::string override { return "null"; }

    [[nodiscard]] auto equals_to(const object_ptr& other) const -> bool override { return other->is(type()); }
};

static null_object null_obj;

struct error_object : object
{
    explicit error_object(std::string msg)
        : message {std::move(msg)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::error; }

    [[nodiscard]] auto inspect() const -> std::string override { return "ERROR: " + message; }

    [[nodiscard]] auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<error_object>()->message == message;
    }

    std::string message;
};

template<typename... T>
auto make_error(fmt::format_string<T...> fmt, T&&... args) -> object_ptr
{
    return make<error_object>(fmt::format(fmt, std::forward<T>(args)...));
}

struct array_object : object
{
    using array = std::vector<object_ptr>;

    explicit array_object(array&& arr)
        : elements {std::move(arr)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::array; }

    [[nodiscard]] auto inspect() const -> std::string override { return "todo"; }

    [[nodiscard]] auto equals_to(const object_ptr& other) const -> bool override
    {
        // FIXME:
        return other->is(type()) && other->as<array_object>()->elements.size() == elements.size();
    }

    array elements;
};

struct hash_object : object
{
    using hash = std::unordered_map<hashable_object::hash_key_type, object_ptr>;

    explicit hash_object(hash&& hsh)
        : pairs {std::move(hsh)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::hash; }

    [[nodiscard]] auto inspect() const -> std::string override { return "todo"; }

    [[nodiscard]] auto equals_to(const object_ptr& other) const -> bool override
    {
        // FIXME:
        return other->is(type()) && other->as<hash_object>()->pairs.size() == pairs.size();
    }

    hash pairs;
};

struct callable_expression;

struct function_object : object
{
    explicit function_object(const callable_expression* expr, environment_ptr env)
        : callable {expr}
        , closure_env {env}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::function; }

    [[nodiscard]] auto inspect() const -> std::string override { return "todo"; }

    [[nodiscard]] auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    const callable_expression* callable;
    environment_ptr closure_env;
};

struct compiled_function_object : object
{
    using ptr = compiled_function_object*;

    compiled_function_object(instructions&& instr, size_t locals, size_t args)
        : instrs {std::move(instr)}
        , num_locals {locals}
        , num_arguments {args}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::compiled_function; }

    [[nodiscard]] auto inspect() const -> std::string override { return "fn<compiled>(...) {...}"; }

    [[nodiscard]] auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    instructions instrs;
    size_t num_locals {};
    size_t num_arguments {};
};

struct closure_object : object
{
    using ptr = closure_object*;

    explicit closure_object(compiled_function_object::ptr cmpld, std::vector<object_ptr> frees = {})
        : fn {cmpld}
        , free {std::move(frees)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::closure; }

    [[nodiscard]] auto inspect() const -> std::string override { return "closure{...}"; }

    [[nodiscard]] auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    compiled_function_object::ptr fn;
    std::vector<object_ptr> free;
};

struct builtin_function_expression;

struct builtin_object : object
{
    explicit builtin_object(const builtin_function_expression* bltn)
        : builtin {bltn}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::builtin; }

    [[nodiscard]] auto inspect() const -> std::string override;

    [[nodiscard]] auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    const builtin_function_expression* builtin;
};
