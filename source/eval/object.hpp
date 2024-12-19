#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <ast/util.hpp>
#include <code/code.hpp>
#include <compiler/symbol_table.hpp>
#include <eval/environment.hpp>
#include <fmt/ostream.h>
#include <gc.hpp>

struct object
{
    enum class object_type : std::uint8_t
    {
        integer,
        decimal,
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
    virtual ~object() = default;
    object(const object&) = delete;
    object(object&&) = delete;
    auto operator=(const object&) -> object& = delete;
    auto operator=(object&&) -> object& = delete;

    [[nodiscard]] auto is(object_type obj_type) const -> bool { return type() == obj_type; }

    template<typename T>
    [[nodiscard]] auto as() const -> const T*
    {
        assert(type() == T {}.type());
        return static_cast<const T*>(this);
    }

    [[nodiscard]] auto is_error() const -> bool { return type() == object_type::error; }

    [[nodiscard]] virtual auto is_truthy() const -> bool { return false; }

    [[nodiscard]] virtual auto is_hashable() const -> bool { return false; }

    [[nodiscard]] virtual auto is_sequence() const -> bool { return false; }

    [[nodiscard]] virtual auto type() const -> object_type = 0;
    [[nodiscard]] virtual auto inspect() const -> std::string = 0;

    [[nodiscard]] virtual auto equals_to(const object* /*other*/) const -> bool { return false; }

    mutable bool is_return_value {};
};

auto operator<<(std::ostream& ostrm, object::object_type type) -> std::ostream&;

template<>
struct fmt::formatter<object::object_type> : ostream_formatter
{
};

auto cast_to_double(const object* obj) -> std::optional<double>;

struct hashable_object : object
{
    using hash_key_type = std::variant<int64_t, std::string, bool>;

    [[nodiscard]] auto is_hashable() const -> bool override { return true; }

    [[nodiscard]] virtual auto hash_key() const -> hash_key_type = 0;
};

auto operator<<(std::ostream& strm, const hashable_object::hash_key_type& t) -> std::ostream&;

struct integer_object : hashable_object
{
    using value_type = std::int64_t;

    explicit integer_object(value_type val)
        : value {val}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return value != 0; }

    [[nodiscard]] auto type() const -> object_type override { return object_type::integer; }

    [[nodiscard]] auto inspect() const -> std::string override { return std::to_string(value); }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        return other->is(type()) && other->as<integer_object>()->value == value;
    }

    value_type value {};
};

struct decimal_object : object
{
    using value_type = double;

    explicit decimal_object(value_type val)
        : value {val}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return value != 0.0; }

    [[nodiscard]] auto type() const -> object_type override { return object_type::decimal; }

    [[nodiscard]] auto inspect() const -> std::string override { return decimal_to_string(value); }

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        return other->is(type()) && other->as<decimal_object>()->value == value;
    }

    value_type value {};
};

struct boolean_object : hashable_object
{
    using value_type = bool;

    explicit boolean_object(value_type val)
        : value {val}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return value; }

    [[nodiscard]] auto type() const -> object_type override { return object_type::boolean; }

    [[nodiscard]] auto inspect() const -> std::string override { return value ? "true" : "false"; }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        return other->is(type()) && other->as<boolean_object>()->value == value;
    }

    value_type value {};
};

auto native_true() -> const object*;
auto native_false() -> const object*;
auto native_bool_to_object(bool val) -> const object*;

struct string_object : hashable_object
{
    using value_type = std::string;

    string_object() = default;

    explicit string_object(value_type val)
        : value {std::move(val)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return !value.empty(); }

    [[nodiscard]] auto type() const -> object_type override { return object_type::string; }

    [[nodiscard]] auto is_sequence() const -> bool override { return true; }

    [[nodiscard]] auto inspect() const -> std::string override { return fmt::format(R"("{}")", value); }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override;

    value_type value;
};

struct null_object : object
{
    [[nodiscard]] auto type() const -> object_type override { return object_type::null; }

    [[nodiscard]] auto inspect() const -> std::string override { return "null"; }

    [[nodiscard]] auto equals_to(const object* other) const -> bool override { return other->is(type()); }
};

auto native_null() -> const object*;

struct error_object : object
{
    error_object() = default;

    explicit error_object(std::string msg)
        : message {std::move(msg)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::error; }

    [[nodiscard]] auto inspect() const -> std::string override { return "ERROR: " + message; }

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        return other->is(type()) && other->as<error_object>()->message == message;
    }

    std::string message;
};

template<typename... T>
auto make_error(fmt::format_string<T...> fmt, T&&... args) -> object*
{
    return make<error_object>(fmt::format(fmt, std::forward<T>(args)...));
}

struct array_object : object
{
    using value_type = std::vector<const object*>;

    array_object() = default;

    explicit array_object(value_type&& arr)
        : value {std::move(arr)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return !value.empty(); }

    [[nodiscard]] auto is_sequence() const -> bool override { return true; }

    [[nodiscard]] auto type() const -> object_type override { return object_type::array; }

    [[nodiscard]] auto inspect() const -> std::string override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override;

    value_type value;
};

struct hash_object : object
{
    using value_type = std::unordered_map<hashable_object::hash_key_type, const object*>;

    explicit hash_object(value_type&& hsh)
        : value {std::move(hsh)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return !value.empty(); }

    [[nodiscard]] auto type() const -> object_type override { return object_type::hash; }

    [[nodiscard]] auto inspect() const -> std::string override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override;

    value_type value;
};

struct callable_expression;

struct function_object : object
{
    function_object(const callable_expression* expr, environment* env)
        : callable {expr}
        , closure_env {env}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::function; }

    [[nodiscard]] auto inspect() const -> std::string override;

    const callable_expression* callable {};
    environment* closure_env {};
};

struct compiled_function_object : object
{
    compiled_function_object(instructions&& instr, size_t locals, size_t args)
        : instrs {std::move(instr)}
        , num_locals {locals}
        , num_arguments {args}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::compiled_function; }

    [[nodiscard]] auto inspect() const -> std::string override { return "fn<compiled>(...) {...}"; }

    instructions instrs;
    size_t num_locals {};
    size_t num_arguments {};
};

struct closure_object : object
{
    explicit closure_object(const compiled_function_object* cmpld, std::vector<const object*> frees = {})
        : fn {cmpld}
        , free {std::move(frees)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::closure; }

    [[nodiscard]] auto inspect() const -> std::string override { return "fn<closure>{...}"; }

    const compiled_function_object* fn {};
    std::vector<const object*> free;
};

struct builtin_function_expression;

struct builtin_object : function_object
{
    explicit builtin_object(const builtin_function_expression* bltn);

    [[nodiscard]] auto type() const -> object_type override { return object_type::builtin; }

    [[nodiscard]] auto inspect() const -> std::string override;

    const builtin_function_expression* builtin {};
};
