#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <ast/identifier.hpp>
#include <ast/statements.hpp>
#include <ast/util.hpp>
#include <code/code.hpp>
#include <compiler/symbol_table.hpp>
#include <eval/environment.hpp>
#include <fmt/ostream.h>
#include <gc.hpp>
#include <sys/types.h>

struct object;
auto tru() -> const object*;
auto fals() -> const object*;
auto object_floor_div(const object* lhs, const object* rhs) -> const object*;
auto native_bool_to_object(bool val) -> const object*;
auto brake() -> const object*;
auto cont() -> const object*;
auto null() -> const object*;

struct hashable
{
    using key_type = std::variant<int64_t, std::string, bool>;

    hashable() = default;
    virtual ~hashable() = default;
    [[nodiscard]] virtual auto hash_key() const -> key_type = 0;

    hashable(const hashable&) = delete;
    hashable(hashable&&) = delete;
    auto operator=(const hashable&) -> hashable& = delete;
    auto operator=(hashable&&) -> hashable& = delete;
};

struct object
{
    enum class object_type : std::uint8_t
    {
        integer,
        decimal,
        boolean,
        string,
        error,
        array,
        hash,
        return_value,
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

    template<typename T>
    [[nodiscard]] auto val() const -> typename T::value_type
    {
        return as<T>()->value;
    }

    [[nodiscard]] auto is_error() const -> bool { return type() == object_type::error; }

    [[nodiscard]] auto is_null() const -> bool { return this == null(); }

    [[nodiscard]] auto is_return_value() const -> bool { return type() == object_type::return_value; }

    [[nodiscard]] auto is_break() const -> bool { return this == brake(); }

    [[nodiscard]] auto is_continue() const -> bool { return this == cont(); }

    [[nodiscard]] virtual auto is_truthy() const -> bool { return false; }

    [[nodiscard]] virtual auto is_hashable() const -> bool { return false; }

    [[nodiscard]] virtual auto type() const -> object_type
    {
        return static_cast<object_type>(std::numeric_limits<uint8_t>::max());
    }

    [[nodiscard]] virtual auto inspect() const -> std::string = 0;

    [[nodiscard]] auto operator!=(const object& other) const -> const object*;
    [[nodiscard]] auto operator&&(const object& other) const -> const object*;
    [[nodiscard]] auto operator||(const object& other) const -> const object*;

    [[nodiscard]] virtual auto operator==(const object& /*other*/) const -> const object* { return fals(); }

    [[nodiscard]] virtual auto operator>(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator>=(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator*(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator+(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator-(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator/(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator%(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator&(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator|(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator^(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator<<(const object& /*other*/) const -> const object* { return nullptr; }

    [[nodiscard]] virtual auto operator>>(const object& /*other*/) const -> const object* { return nullptr; }
};

template<>
[[nodiscard]] inline auto object::as() const -> const hashable*
{
    return dynamic_cast<const hashable*>(this);
}

auto operator<<(std::ostream& ostrm, object::object_type type) -> std::ostream&;

template<>
struct fmt::formatter<object::object_type> : ostream_formatter
{
};

auto operator<<(std::ostream& strm, const hashable::key_type& t) -> std::ostream&;

struct integer_object final
    : object
    , hashable
{
    using value_type = std::int64_t;

    integer_object() = default;

    explicit integer_object(value_type val)
        : value {val}
    {
    }

    template<typename T>
    [[nodiscard]] auto value_to() const -> typename T::value_type
    {
        return static_cast<typename T::value_type>(val<integer_object>());
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return value != 0; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::integer; }

    [[nodiscard]] auto inspect() const -> std::string final { return std::to_string(value); }

    [[nodiscard]] auto is_hashable() const -> bool final { return true; }

    [[nodiscard]] auto hash_key() const -> key_type final;

    [[nodiscard]] auto operator==(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>=(const object& other) const -> const object* final;
    [[nodiscard]] auto operator+(const object& other) const -> const object* final;
    [[nodiscard]] auto operator-(const object& other) const -> const object* final;
    [[nodiscard]] auto operator*(const object& other) const -> const object* final;
    [[nodiscard]] auto operator/(const object& other) const -> const object* final;
    [[nodiscard]] auto operator%(const object& other) const -> const object* final;
    // TODO: use unsigned for all the bit ops
    [[nodiscard]] auto operator&(const object& other) const -> const object* final;
    [[nodiscard]] auto operator|(const object& other) const -> const object* final;
    [[nodiscard]] auto operator^(const object& other) const -> const object* final;
    [[nodiscard]] auto operator<<(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>>(const object& other) const -> const object* final;

    value_type value {};
};

struct decimal_object final : object
{
    using value_type = double;

    decimal_object() = default;

    explicit decimal_object(value_type val)
        : value {val}
    {
    }

    template<typename T>
    [[nodiscard]] auto value_to() const -> typename T::value_type
    {
        return static_cast<typename T::value_type>(val<decimal_object>());
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return value != 0.0; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::decimal; }

    [[nodiscard]] auto inspect() const -> std::string final { return decimal_to_string(value); }

    [[nodiscard]] auto operator==(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>=(const object& other) const -> const object* final;
    [[nodiscard]] auto operator+(const object& other) const -> const object* final;
    [[nodiscard]] auto operator-(const object& other) const -> const object* final;
    [[nodiscard]] auto operator*(const object& other) const -> const object* final;
    [[nodiscard]] auto operator/(const object& other) const -> const object* final;
    [[nodiscard]] auto operator%(const object& other) const -> const object* final;

    value_type value {};
};

struct boolean_object final
    : object
    , hashable
{
    using value_type = bool;

    explicit boolean_object(value_type val)
        : value {val}
    {
    }

    template<typename T>
    [[nodiscard]] auto value_to() const -> typename T::value_type
    {
        return static_cast<typename T::value_type>(val<boolean_object>());
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return value; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::boolean; }

    [[nodiscard]] auto inspect() const -> std::string final { return value ? "true" : "false"; }

    [[nodiscard]] auto is_hashable() const -> bool final { return true; }

    [[nodiscard]] auto hash_key() const -> key_type final;
    [[nodiscard]] auto operator==(const object& other) const -> const object* final;
    [[nodiscard]] auto operator+(const object& other) const -> const object* final;
    [[nodiscard]] auto operator-(const object& other) const -> const object* final;
    [[nodiscard]] auto operator*(const object& other) const -> const object* final;
    [[nodiscard]] auto operator/(const object& other) const -> const object* final;
    [[nodiscard]] auto operator%(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>=(const object& other) const -> const object* final;
    [[nodiscard]] auto operator&(const object& other) const -> const object* final;
    [[nodiscard]] auto operator|(const object& other) const -> const object* final;
    [[nodiscard]] auto operator^(const object& other) const -> const object* final;
    [[nodiscard]] auto operator<<(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>>(const object& other) const -> const object* final;

    value_type value {};
};

struct string_object final
    : object
    , hashable
{
    using value_type = std::string;

    string_object() = default;

    explicit string_object(value_type val)
        : value {std::move(val)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return !value.empty(); }

    [[nodiscard]] auto type() const -> object_type final { return object_type::string; }

    [[nodiscard]] auto inspect() const -> std::string final { return fmt::format(R"("{}")", value); }

    [[nodiscard]] auto is_hashable() const -> bool final { return true; }

    [[nodiscard]] auto hash_key() const -> key_type final;
    [[nodiscard]] auto operator==(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>(const object& other) const -> const object* final;
    [[nodiscard]] auto operator>=(const object& other) const -> const object* final;
    [[nodiscard]] auto operator+(const object& other) const -> const object* final;
    [[nodiscard]] auto operator*(const object& other) const -> const object* final;

    value_type value;
};

struct break_object final : object
{
    [[nodiscard]] auto inspect() const -> std::string final { return "break"; }
};

struct continue_object final : object
{
    [[nodiscard]] auto inspect() const -> std::string final { return "continue"; }
};

struct null_object final : object
{
    [[nodiscard]] auto inspect() const -> std::string final { return "null"; }

    [[nodiscard]] auto operator==(const object& other) const -> const object* final;
};

struct error_object final : object
{
    explicit error_object(std::string msg)
        : value {std::move(msg)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return true; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::error; }

    [[nodiscard]] auto inspect() const -> std::string final { return "ERROR: " + value; }

    [[nodiscard]] auto operator==(const object& other) const -> const object* final;

    std::string value;
};

template<typename... T>
auto make_error(fmt::format_string<T...> fmt, T&&... args) -> object*
{
    return make<error_object>(fmt::format(fmt, std::forward<T>(args)...));
}

struct array_object final : object
{
    using value_type = std::vector<const object*>;

    array_object() = default;

    explicit array_object(value_type&& arr)
        : value {std::move(arr)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return !value.empty(); }

    [[nodiscard]] auto type() const -> object_type final { return object_type::array; }

    [[nodiscard]] auto inspect() const -> std::string final;
    [[nodiscard]] auto operator==(const object& other) const -> const object* final;
    [[nodiscard]] auto operator*(const object& /*other*/) const -> const object* final;
    [[nodiscard]] auto operator+(const object& other) const -> const object* final;

    value_type value;
};

struct hash_object final : object
{
    using value_type = std::unordered_map<hashable::key_type, const object*>;

    explicit hash_object(value_type&& hsh)
        : value {std::move(hsh)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return !value.empty(); }

    [[nodiscard]] auto type() const -> object_type final { return object_type::hash; }

    [[nodiscard]] auto inspect() const -> std::string final;
    [[nodiscard]] auto operator==(const object& other) const -> const object* final;
    [[nodiscard]] auto operator+(const object& other) const -> const object* final;

    value_type value;
};

struct return_value_object final : object
{
    explicit return_value_object(const object* obj)
        : return_value {obj}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return return_value->is_truthy(); }

    [[nodiscard]] auto type() const -> object_type final { return object_type::return_value; }

    [[nodiscard]] auto inspect() const -> std::string final;

    const object* return_value;
};

struct function_object final : object
{
    function_object(const std::vector<const identifier*>& params, const block_statement* bod, environment* env)
        : parameters {params}
        , body {bod}
        , closure_env {env}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return true; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::function; }

    [[nodiscard]] auto inspect() const -> std::string final;

    std::vector<const identifier*> parameters;
    const block_statement* body {};
    environment* closure_env {};
};

struct compiled_function_object final : object
{
    compiled_function_object(instructions&& instr, int locals, int args)
        : instrs {std::move(instr)}
        , num_locals {locals}
        , num_arguments {args}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return true; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::compiled_function; }

    [[nodiscard]] auto inspect() const -> std::string final;

    instructions instrs;
    int num_locals {};
    int num_arguments {};
    bool inside_loop {};
};

struct closure_object final : object
{
    explicit closure_object(const compiled_function_object* cmpld, std::vector<const object*> frees = {})
        : fn {cmpld}
        , free {std::move(frees)}
    {
    }

    [[nodiscard]] auto is_truthy() const -> bool final { return true; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::closure; }

    [[nodiscard]] auto inspect() const -> std::string final;
    [[nodiscard]] auto as_mutable() const -> closure_object*;

    const compiled_function_object* fn {};
    std::vector<const object*> free;
};

struct builtin_object final : object
{
    explicit builtin_object(const struct builtin* bltn);

    [[nodiscard]] auto is_truthy() const -> bool final { return true; }

    [[nodiscard]] auto type() const -> object_type final { return object_type::builtin; }

    [[nodiscard]] auto inspect() const -> std::string final;

    const struct builtin* builtin {};
};
