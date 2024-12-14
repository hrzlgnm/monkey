#pragma once

#include <algorithm>
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

#include "eval/environment.hpp"

// helper type for std::visit
template<typename... T>
struct overloaded : T...
{
    using T::operator()...;
};
template<class... T>
overloaded(T...) -> overloaded<T...>;

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
    virtual ~object() = default;
    object(const object&) = delete;
    object(object&&) = delete;
    auto operator=(const object&) -> object& = delete;
    auto operator=(object&&) -> object& = delete;

    [[nodiscard]] auto is(object_type obj_type) const -> bool { return type() == obj_type; }

    template<typename T>
    [[nodiscard]] auto as() const -> const T*
    {
        return static_cast<const T*>(this);
    }

    [[nodiscard]] auto is_error() const -> bool { return type() == object_type::error; }

    [[nodiscard]] virtual auto is_truthy() const -> bool { return true; }

    [[nodiscard]] virtual auto is_hashable() const -> bool { return false; }

    [[nodiscard]] virtual auto type() const -> object_type = 0;
    [[nodiscard]] virtual auto inspect() const -> std::string = 0;

    [[nodiscard]] virtual auto equals_to(const object* /*other*/) const -> bool { return false; };

    mutable bool is_return_value {};
};

auto operator<<(std::ostream& ostrm, object::object_type type) -> std::ostream&;

template<>
struct fmt::formatter<object::object_type> : ostream_formatter
{
};

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

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
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

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        return other->is(type()) && other->as<boolean_object>()->value == value;
    }

    bool value {};
};

auto native_true() -> const object*;
auto native_false() -> const object*;

inline auto native_bool_to_object(bool val) -> const object*
{
    if (val) {
        return native_true();
    }
    return native_false();
}

struct string_object : hashable_object
{
    explicit string_object(std::string val)
        : value {std::move(val)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::string; }

    [[nodiscard]] auto inspect() const -> std::string override { return fmt::format(R"("{}")", value); }

    [[nodiscard]] auto hash_key() const -> hash_key_type override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        return other->is(type()) && other->as<string_object>()->value == value;
    }

    std::string value;
};

struct null_object : object
{
    [[nodiscard]] auto is_truthy() const -> bool override { return false; }

    [[nodiscard]] auto type() const -> object_type override { return object_type::null; }

    [[nodiscard]] auto inspect() const -> std::string override { return "null"; }

    [[nodiscard]] auto equals_to(const object* other) const -> bool override { return other->is(type()); }
};

auto native_null() -> const object*;

struct error_object : object
{
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
    using array = std::vector<const object*>;

    explicit array_object(array&& arr)
        : elements {std::move(arr)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::array; }

    [[nodiscard]] auto inspect() const -> std::string override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        if (!other->is(type()) || other->as<array_object>()->elements.size() != elements.size()) {
            return false;
        }
        const auto& other_elements = other->as<array_object>()->elements;
        return std::equal(elements.cbegin(),
                          elements.cend(),
                          other_elements.cbegin(),
                          other_elements.cend(),
                          [](const object* a, const object* b) { return a->equals_to(b); });
    }

    array elements;
};

struct hash_object : object
{
    using hash = std::unordered_map<hashable_object::hash_key_type, const object*>;

    explicit hash_object(hash&& hsh)
        : pairs {std::move(hsh)}
    {
    }

    [[nodiscard]] auto type() const -> object_type override { return object_type::hash; }

    [[nodiscard]] auto inspect() const -> std::string override;

    [[nodiscard]] auto equals_to(const object* other) const -> bool override
    {
        if (!other->is(type()) || other->as<hash_object>()->pairs.size() != pairs.size()) {
            return false;
        }
        const auto& other_pairs = other->as<hash_object>()->pairs;
        return std::all_of(pairs.cbegin(),
                           pairs.cend(),
                           [other_pairs](const auto& pair)
                           {
                               const auto& [key, value] = pair;
                               auto it = other_pairs.find(key);
                               return it != other_pairs.cend() && it->second->equals_to(value);
                           });
    }

    hash pairs;
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

    [[nodiscard]] auto inspect() const -> std::string override { return "closure{...}"; }

    const compiled_function_object* fn {};
    std::vector<const object*> free;
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

    const builtin_function_expression* builtin {};
};
