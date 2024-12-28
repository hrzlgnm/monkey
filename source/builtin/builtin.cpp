#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "builtin.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <gc.hpp>
#include <object/object.hpp>

builtin::builtin(std::string name,
                 std::vector<std::string> params,
                 std::function<const object*(array_object::value_type&& arguments)> bod)
    : name {std::move(name)}
    , parameters {std::move(params)}
    , body {std::move(bod)}
{
}

namespace
{

const builtin len {
    "len",
    {"val"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to len(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array_or_hash = arguments[0];
        using enum object::object_type;
        if (maybe_string_or_array_or_hash->is(string)) {
            const auto& str = maybe_string_or_array_or_hash->as<string_object>()->value;
            return make<integer_object>(static_cast<int64_t>(str.size()));
        }
        if (maybe_string_or_array_or_hash->is(array)) {
            const auto& arr = maybe_string_or_array_or_hash->as<array_object>()->value;

            return make<integer_object>(static_cast<int64_t>(arr.size()));
        }
        if (maybe_string_or_array_or_hash->is(hash)) {
            const auto& hsh = maybe_string_or_array_or_hash->as<hash_object>()->value;

            return make<integer_object>(static_cast<int64_t>(hsh.size()));
        }
        return make_error("argument of type {} to len() is not supported", maybe_string_or_array_or_hash->type());
    }};

const builtin pts {"puts",
                   {"val..."},
                   [](const array_object::value_type& arguments) -> const object*
                   {
                       using enum object::object_type;
                       for (bool first = true; const auto& arg : arguments) {
                           if (!first) {
                               fmt::print(" ");
                           }
                           if (arg->is(string)) {
                               fmt::print("{}", arg->as<string_object>()->value);
                           } else {
                               fmt::print("{}", arg->inspect());
                           }
                           first = false;
                       }
                       fmt::print("\n");
                       return null();
                   }};

const builtin first {
    "first",
    {"arr|str"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to first(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array = arguments.at(0);
        using enum object::object_type;
        if (maybe_string_or_array->is(string)) {
            const auto& str = maybe_string_or_array->as<string_object>()->value;
            if (!str.empty()) {
                return make<string_object>(str.substr(0, 1));
            }
            return null();
        }
        if (maybe_string_or_array->is(array)) {
            const auto& arr = maybe_string_or_array->as<array_object>()->value;
            if (!arr.empty()) {
                return arr.front();
            }
            return null();
        }
        return make_error("argument of type {} to first() is not supported", maybe_string_or_array->type());
    }};

const builtin last {
    "last",
    {"arr|str"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to last(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array = arguments[0];
        using enum object::object_type;
        if (maybe_string_or_array->is(string)) {
            const auto& str = maybe_string_or_array->as<string_object>()->value;
            if (!str.empty()) {
                return make<string_object>(str.substr(str.length() - 1, 1));
            }
            return null();
        }
        if (maybe_string_or_array->is(array)) {
            const auto& arr = maybe_string_or_array->as<array_object>()->value;
            if (!arr.empty()) {
                return arr.back();
            }
            return null();
        }
        return make_error("argument of type {} to last() is not supported", maybe_string_or_array->type());
    }};

const builtin rest {
    "rest",
    {"arr|str"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to rest(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array = arguments.at(0);
        using enum object::object_type;
        if (maybe_string_or_array->is(string)) {
            const auto& str = maybe_string_or_array->as<string_object>()->value;
            if (str.size() > 1) {
                return make<string_object>(str.substr(1));
            }
            return null();
        }
        if (maybe_string_or_array->is(array)) {
            const auto& arr = maybe_string_or_array->as<array_object>()->value;
            if (arr.size() > 1) {
                array_object::value_type rest;
                std::copy(arr.cbegin() + 1, arr.cend(), std::back_inserter(rest));
                return make<array_object>(std::move(rest));
            }
            return null();
        }
        return make_error("argument of type {} to rest() is not supported", maybe_string_or_array->type());
    }};

const builtin push {
    "push",
    {"arr|str|hsh", "val|str|hashable", "val"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 2 && arguments.size() != 3) {
            return make_error("wrong number of arguments to push(): expected=2 or 3, got={}", arguments.size());
        }
        using enum object::object_type;
        if (arguments.size() == 2) {
            const auto& lhs = arguments[0];
            const auto& rhs = arguments[1];
            if (lhs->is(array)) {
                auto copy = lhs->as<array_object>()->value;
                copy.push_back(rhs);
                return make<array_object>(std::move(copy));
            }
            if (lhs->is(string) && rhs->is(string)) {
                auto copy = lhs->as<string_object>()->value;
                copy.append(rhs->as<string_object>()->value);
                return make<string_object>(std::move(copy));
            }
            return make_error("argument of type {} and {} to push() are not supported", lhs->type(), rhs->type());
        }
        if (arguments.size() == 3) {
            const auto& lhs = arguments[0];
            const auto& k = arguments[1];
            const auto& v = arguments[2];
            if (lhs->is(hash)) {
                if (!k->is_hashable()) {
                    return make_error("type {} is not hashable", k->type());
                }
                auto copy = lhs->as<hash_object>()->value;
                copy.insert_or_assign(k->as<hashable>()->hash_key(), v);
                return make<hash_object>(std::move(copy));
            }
            return make_error(
                "argument of type {}, {} and {} to push() are not supported", lhs->type(), k->type(), v->type());
        }
        return make_error("invalid call to push()");
    }};

const builtin type {"type",
                    {"val"},
                    [](const array_object::value_type& arguments) -> const object*
                    {
                        if (arguments.size() != 1) {
                            return make_error("wrong number of arguments to type(): expected=1, got={}",
                                              arguments.size());
                        }
                        const auto& val = arguments[0];
                        return make<string_object>(fmt::format("{}", val->type()));
                    }};
const builtin chr {"chr",
                   {"int"},
                   [](const array_object::value_type& arguments) -> const object*
                   {
                       if (arguments.size() != 1) {
                           return make_error("wrong number of arguments to chr(): expected=1, got={}",
                                             arguments.size());
                       }
                       const auto* val = arguments[0];
                       if (val->is(object::object_type::integer)) {
                           const auto& as_int = val->as<integer_object>()->value;
                           if (isascii(static_cast<int>(as_int))) {
                               return make<string_object>(fmt::format("{}", static_cast<char>(as_int)));
                           }
                           return make_error("number {} is out of range to be an ascii character", as_int);
                       }
                       return make_error("argument of type {} to chr() is not supported", val->type());
                   }};
}  // namespace

auto builtin::builtins() -> const std::vector<const builtin*>&
{
    static const std::vector<const builtin*> bltns {&len, &pts, &first, &last, &rest, &push, &type, &chr};
    return bltns;
};
