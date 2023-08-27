#pragma once

#include <algorithm>
#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

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
using object_ptr = std::shared_ptr<object>;

struct object : std::enable_shared_from_this<object>
{
    enum class object_type
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

    [[nodiscard]] inline auto is(object_type obj_type) const -> bool { return type() == obj_type; }

    template<typename T>
    [[nodiscard]] inline auto as() -> std::shared_ptr<T>
    {
        return std::static_pointer_cast<T>(shared_from_this());
    }

    [[nodiscard]] inline auto is_error() const -> bool { return type() == object_type::error; }

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

    [[nodiscard]] inline auto is_hashable() const -> bool override { return true; }

    [[nodiscard]] virtual auto hash_key() const -> hash_key_type = 0;
};

struct integer_object : hashable_object
{
    explicit inline integer_object(int64_t val)
        : value {val}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::integer; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return std::to_string(value); }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] inline auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<integer_object>()->value == value;
    }

    int64_t value {};
};

struct boolean_object : hashable_object
{
    explicit inline boolean_object(bool val)
        : value {val}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool override { return value; }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::boolean; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return value ? "true" : "false"; }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] inline auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<boolean_object>()->value == value;
    }

    bool value {};
};

static const object_ptr false_object {std::make_shared<boolean_object>(false)};
static const object_ptr true_object {std::make_shared<boolean_object>(true)};

inline auto native_bool_to_object(bool val) -> object_ptr
{
    if (val) {
        return true_object;
    }
    return false_object;
}

struct string_object : hashable_object
{
    explicit inline string_object(std::string val)
        : value {std::move(val)}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::string; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return value; }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] inline auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<string_object>()->value == value;
    }

    std::string value;
};

struct null_object : object
{
    null_object() = default;

    [[nodiscard]] auto is_truthy() const -> bool override { return false; }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::null; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return "null"; }

    [[nodiscard]] inline auto equals_to(const object_ptr& other) const -> bool override { return other->is(type()); }
};

static const object_ptr null {std::make_shared<null_object>()};

struct error_object : object
{
    explicit inline error_object(std::string msg)
        : message {std::move(msg)}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::error; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return "ERROR: " + message; }

    [[nodiscard]] inline auto equals_to(const object_ptr& other) const -> bool override
    {
        return other->is(type()) && other->as<error_object>()->message == message;
    }

    std::string message;
};

template<typename... T>
auto make_error(fmt::format_string<T...> fmt, T&&... args) -> object_ptr
{
    return std::make_unique<error_object>(fmt::format(fmt, std::forward<T>(args)...));
}

struct array_object : object
{
    using array = std::vector<object_ptr>;

    inline explicit array_object(array&& arr)
        : elements {std::move(arr)}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::array; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return "todo"; }

    [[nodiscard]] inline auto equals_to(const object_ptr& other) const -> bool override
    {
        // FIXME:
        return other->is(type()) && other->as<array_object>()->elements.size() == elements.size();
    }

    array elements;
};

struct hash_object : object
{
    using hash = std::unordered_map<hashable_object::hash_key_type, object_ptr>;

    inline explicit hash_object(hash&& hsh)
        : pairs {std::move(hsh)}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::hash; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return "todo"; }

    [[nodiscard]] inline auto equals_to(const object_ptr& other) const -> bool override
    {
        // FIXME:
        return other->is(type()) && other->as<hash_object>()->pairs.size() == pairs.size();
    }

    hash pairs;
};

struct callable_expression;

struct function_object : object
{
    inline explicit function_object(const callable_expression* expr, environment_ptr env)
        : callable {expr}
        , closure_env {std::move(env)}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::function; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return "todo"; }

    [[nodiscard]] inline auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    const callable_expression* callable;
    environment_ptr closure_env;
};

struct compiled_function_object : object
{
    using ptr = std::shared_ptr<compiled_function_object>;

    inline compiled_function_object(instructions&& instr, size_t locals, size_t args)
        : instrs {std::move(instr)}
        , num_locals {locals}
        , num_arguments {args}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::compiled_function; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return "fn<compiled>(...) {...}"; }

    [[nodiscard]] inline auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    instructions instrs;
    size_t num_locals {};
    size_t num_arguments {};
};

struct closure_object : object
{
    using ptr = std::shared_ptr<closure_object>;

    inline explicit closure_object(compiled_function_object::ptr cmpld, std::vector<object_ptr> frees = {})
        : fn {std::move(cmpld)}
        , free {std::move(frees)}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::closure; }

    [[nodiscard]] inline auto inspect() const -> std::string override { return "closure{...}"; }

    [[nodiscard]] inline auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    compiled_function_object::ptr fn;
    std::vector<object_ptr> free;
};

struct builtin_function_expression;

struct builtin_object : object
{
    inline explicit builtin_object(const builtin_function_expression* bltn)
        : builtin {bltn}
    {
    }

    [[nodiscard]] inline auto type() const -> object_type override { return object_type::builtin; }

    [[nodiscard]] auto inspect() const -> std::string override;

    [[nodiscard]] inline auto equals_to(const object_ptr& /*other*/) const -> bool override { return false; }

    const builtin_function_expression* builtin;
};
