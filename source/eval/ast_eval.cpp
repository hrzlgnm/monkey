#include <deque>

#include <ast/array_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean.hpp>
#include <ast/builtin_function_expression.hpp>
#include <ast/call_expression.hpp>
#include <ast/character_literal.hpp>
#include <ast/function_expression.hpp>
#include <ast/hash_literal_expression.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/program.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>
#include <doctest/doctest.h>
#include <parser/parser.hpp>

#include "environment.hpp"
#include "object.hpp"

auto array_expression::eval(environment_ptr env) const -> object
{
    array result;
    for (const auto& element : elements) {
        auto evaluated = element->eval(env);
        if (evaluated.is<error>()) {
            return evaluated;
        }
        result.push_back(std::move(evaluated));
    }
    return {result};
}

auto eval_integer_binary_expression(token_type oper, const int64_t left, const int64_t right) -> object
{
    using enum token_type;
    switch (oper) {
        case plus:
            return {left + right};
        case minus:
            return {left - right};
        case asterisk:
            return {left * right};
        case slash:
            return {left / right};
        case less_than:
            return {left < right};
        case greater_than:
            return {left > right};
        case equals:
            return {left == right};
        case not_equals:
            return {left != right};
        default:
            return {};
    }
}

auto eval_character_binary_expression(token_type oper, const char left, const char right) -> object
{
    using enum token_type;
    switch (oper) {
        case plus: {
            string_type res;
            res.push_back(left);
            res.push_back(right);
            return {res};
        }
        case less_than:
            return {left < right};
        case greater_than:
            return {left > right};
        case equals:
            return {left == right};
        case not_equals:
            return {left != right};
        default:
            return make_error("unknown operator: character {} character", oper);
    }
}

auto eval_string_binary_expression(token_type oper, const string_type& left, const string_type& right) -> object
{
    using enum token_type;
    if (oper != plus) {
        return make_error("unknown operator: string {} string", oper);
    }
    return {left + right};
}

auto binary_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_left = left->eval(env);
    if (evaluated_left.is<error>()) {
        return evaluated_left;
    }

    auto evaluated_right = right->eval(env);
    if (evaluated_right.is<error>()) {
        return evaluated_right;
    }

    if (evaluated_left.is<char>() && evaluated_right.is<char>()) {
        return eval_character_binary_expression(op, evaluated_left.as<char>(), evaluated_right.as<char>());
    }
    if (evaluated_left.is<string_type>() && evaluated_right.is<string_type>()) {
        return eval_string_binary_expression(op, evaluated_left.as<string_type>(), evaluated_right.as<string_type>());
    }
    if ((evaluated_left.is<char>() || evaluated_left.is<string_type>())
        && (evaluated_right.is<string_type>() || evaluated_right.is<char>()))
    {
        return eval_string_binary_expression(op, evaluated_left.to<string_type>(), evaluated_right.to<string_type>());
    }
    if (evaluated_left.is<integer_type>() && evaluated_right.is<integer_type>()) {
        return eval_integer_binary_expression(
            op, evaluated_left.as<integer_type>(), evaluated_right.as<integer_type>());
    }
    if (evaluated_left.value.index() != evaluated_right.value.index()) {
        return make_error("type mismatch: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name());
    }
    return make_error("unknown operator: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name());
}

auto boolean::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}

auto function_expression::call(environment_ptr closure_env,
                               environment_ptr caller_env,
                               const std::vector<expression_ptr>& arguments) const -> object
{
    auto locals = std::make_shared<environment>(closure_env);
    for (auto arg_itr = arguments.begin(); const auto& parameter : parameters) {
        if (arg_itr != arguments.end()) {
            const auto& arg = *(arg_itr++);
            locals->set(parameter, arg->eval(caller_env));
        } else {
            locals->set(parameter, {});
        }
    }
    return body->eval(locals);
}

auto call_expression::eval(environment_ptr env) const -> object
{
    auto evaluated = function->eval(env);
    if (evaluated.is<error>()) {
        return evaluated;
    }
    auto [fn, closure_env] = evaluated.as<bound_function>();
    return fn->call(closure_env, env, arguments);
}

auto hash_literal_expression::eval(environment_ptr env) const -> object
{
    hash result;

    for (const auto& [key, value] : pairs) {
        auto eval_key = key->eval(env);
        if (eval_key.is<error>()) {
            return eval_key;
        }
        if (!eval_key.is_hashable()) {
            return make_error("unusable as hash key {}", eval_key.type_name());
        }
        auto eval_val = value->eval(env);
        if (eval_val.is<error>()) {
            return eval_val;
        }
        result.insert({eval_key.hash_key(), std::make_any<object>(eval_val)});
    }
    return {result};
}

auto identifier::eval(environment_ptr env) const -> object
{
    auto val = env->get(value);
    if (val.is_nil()) {
        return make_error("identifier not found: {}", value);
    }
    return val;
}

auto if_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_condition = condition->eval(env);
    if (evaluated_condition.is<error>()) {
        return evaluated_condition;
    }
    if (evaluated_condition.is_truthy()) {
        return consequence->eval(env);
    }
    if (alternative) {
        return alternative->eval(env);
    }
    return {};
}

auto index_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_left = left->eval(env);
    if (evaluated_left.is<error>()) {
        return evaluated_left;
    }
    auto evaluated_index = index->eval(env);
    if (evaluated_index.is<error>()) {
        return evaluated_index;
    }
    if (evaluated_left.is<array>() && evaluated_index.is<integer_type>()) {
        auto arr = evaluated_left.as<array>();
        auto index = evaluated_index.as<integer_type>();
        auto max = static_cast<int64_t>(arr.size() - 1);
        if (index < 0 || index > max) {
            return {};
        }
        return arr.at(static_cast<size_t>(index));
    }

    if (evaluated_left.is<string_type>() && evaluated_index.is<integer_type>()) {
        auto str = evaluated_left.as<string_type>();
        auto index = evaluated_index.as<integer_type>();
        auto max = static_cast<int64_t>(str.size() - 1);
        if (index < 0 || index > max) {
            return {};
        }
        return {str.at(static_cast<size_t>(index))};
    }

    if (evaluated_left.is<hash>()) {
        auto hsh = evaluated_left.as<hash>();
        if (!evaluated_index.is_hashable()) {
            return make_error("unusable as hash key: {}", evaluated_index.type_name());
        }
        if (!hsh.contains(evaluated_index.hash_key())) {
            return {nil};
        }
        return unwrap(hsh.at(evaluated_index.hash_key()));
    }
    return make_error("index operator not supported: {}", evaluated_left.type_name());
}

auto integer_literal::eval(environment_ptr /*env*/) const -> object
{
    return {value};
}

auto program::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& statement : statements) {
        result = statement->eval(env);
        if (result.is_return_value) {
            return result;
        }
        if (result.is<error>()) {
            return result;
        }
    }
    return result;
}

auto let_statement::eval(environment_ptr env) const -> object
{
    auto val = value->eval(env);
    if (val.is<error>()) {
        return val;
    }
    env->set(name->value, std::move(val));
    return nil;
}

auto return_statement::eval(environment_ptr env) const -> object
{
    if (value) {
        auto evaluated = value->eval(env);
        if (evaluated.is<error>()) {
            return evaluated;
        }
        evaluated.is_return_value = true;
        return evaluated;
    }
    return nil;
}

auto expression_statement::eval(environment_ptr env) const -> object
{
    if (expr) {
        return expr->eval(env);
    }
    return nil;
}

auto block_statement::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& stmt : statements) {
        result = stmt->eval(env);
        if (result.is_return_value || result.is<error>()) {
            return result;
        }
    }
    return result;
}

auto string_literal::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}

auto unary_expression::eval(environment_ptr env) const -> object
{
    using enum token_type;
    auto evaluated_value = right->eval(env);
    if (evaluated_value.is<error>()) {
        return evaluated_value;
    }
    switch (op) {
        case minus:
            if (!evaluated_value.is<integer_type>()) {
                return make_error("unknown operator: -{}", evaluated_value.type_name());
            }
            return {-evaluated_value.as<integer_type>()};
        case exclamation:
            if (!evaluated_value.is<bool>()) {
                return {false};
            }
            return {!evaluated_value.as<bool>()};
        default:
            return make_error("unknown operator: {}{}", op, evaluated_value.type_name());
    }
}

auto builtin_function_expression::call(environment_ptr /*closure_env*/,
                                       environment_ptr caller_env,
                                       const std::vector<expression_ptr>& arguments) const -> object
{
    array args;
    std::transform(arguments.cbegin(),
                   arguments.cend(),
                   std::back_inserter(args),
                   [&caller_env](const expression_ptr& expr) { return expr->eval(caller_env); });
    return body(std::move(args));
}

const std::vector<builtin_function_expression> builtin_function_expression::builtins {
    {"len",
     {"val"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to len(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object { return {static_cast<integer_type>(str.length())}; },
                 [](const array& arr) -> object { return {static_cast<integer_type>(arr.size())}; },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to len() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"puts",
     {"str"},
     [](const array& arguments) -> object
     {
         for (bool first = true; const auto& arg : arguments) {
             if (!first) {
                 fmt::print(" ");
             }
             if (arg.is<string_type>()) {
                 fmt::print("{}", arg.as<string_type>());
             } else {
                 fmt::print("{}", std::to_string(arg.value));
             }
             first = false;
         }
         fmt::print("\n");
         return {nil};
     }},
    {"first",
     {"arr"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to first(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object
                 {
                     if (str.length() > 0) {
                         return {str.at(0)};
                     }
                     return {};
                 },
                 [](const array& arr) -> object
                 {
                     if (!arr.empty()) {
                         return arr.front();
                     }
                     return {};
                 },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to first() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"last",
     {"arr"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to last(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object
                 {
                     if (str.length() > 0) {
                         return {str.at(str.length() - 1)};
                     }
                     return {};
                 },
                 [](const array& arr) -> object
                 {
                     if (!arr.empty()) {
                         return arr.back();
                     }
                     return {};
                 },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to last() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"rest",
     {"arr"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to rest(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object
                 {
                     if (str.size() > 1) {
                         return {str.substr(1)};
                     }
                     return {};
                 },
                 [](const array& arr) -> object
                 {
                     if (arr.size() > 1) {
                         array rest;
                         std::copy(arr.begin() + 1, arr.end(), std::back_inserter(rest));
                         return {rest};
                     }
                     return {};
                 },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to rest() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"push",
     {"arr", "val"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 2) {
             return make_error("wrong number of arguments to push(): expected=2, got={}", arguments.size());
         }
         return std::visit(
             overloaded {
                 [](const array& arr, const auto& obj) -> object
                 {
                     auto copy = arr;
                     copy.push_back({obj});
                     return {copy};
                 },
                 [](const string_type& str, const char obj) -> object
                 {
                     auto copy = str;
                     copy.push_back(obj);
                     return {copy};
                 },
                 [](const auto& other1, const auto& other2) -> object
                 {
                     return make_error("argument of type {} and {} to push() are not supported",
                                       object {other1}.type_name(),
                                       object {other2}.type_name());
                 },
             },
             arguments[0].value,
             arguments[1].value);
     }},
};

auto callable_expression::eval(environment_ptr env) const -> object
{
    return {std::make_pair(this, env)};
}

auto character_literal::eval(environment_ptr /*env*/) const -> object
{
    return {value};
}

namespace
{
// NOLINTBEGIN(*)
template<typename Expected>
auto require_eq(const object& obj, const Expected& expected) -> void
{
    INFO("expected: ",
         object {expected}.type_name(),
         " with: ",
         std::to_string(expected),
         " got: ",
         obj.type_name(),
         " with: ",
         std::to_string(obj.value));
    REQUIRE(obj.is<Expected>());
    const auto& actual = obj.as<Expected>();
    REQUIRE(actual == expected);
}

auto check_no_parse_errors(const parser& prsr) -> bool
{
    INFO("expected no errors, got:", fmt::format("{}", fmt::join(prsr.errors(), ", ")));
    CHECK(prsr.errors().empty());
    return prsr.errors().empty();
}

using parsed_program = std::pair<program_ptr, parser>;

auto check_program(std::string_view input) -> parsed_program
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    INFO("while parsing: `", input, "`");
    CHECK(check_no_parse_errors(prsr));
    return {std::move(prgrm), std::move(prsr)};
}

auto run(std::string_view input) -> object
{
    auto [prgrm, _] = check_program(input);
    auto env = std::make_shared<environment>();
    for (const auto& builtin : builtin_function_expression::builtins) {
        env->set(builtin.name, object {bound_function(&builtin, environment_ptr {})});
    }

    auto result = prgrm->eval(env);
    env->break_cycle();
    return result;
}

auto run_multi(std::deque<std::string>& inputs) -> object
{
    auto globals = std::make_shared<environment>();
    auto statements = std::vector<statement_ptr>();
    object result;
    while (!inputs.empty()) {
        auto [prgrm, _] = check_program(inputs.front());
        result = prgrm->eval(globals);
        for (auto& stmt : prgrm->statements) {
            statements.push_back(std::move(stmt));
        }
        inputs.pop_front();
    }
    globals->break_cycle();
    return result;
}

TEST_SUITE_BEGIN("eval");

TEST_CASE("integerExpresssion")
{
    struct et
    {
        std::string_view input;
        int64_t expected;
    };

    std::array tests {
        et {"5", 5},
        et {"10", 10},
        et {"-5", -5},
        et {"-10", -10},
        et {"5 + 5 + 5 + 5 - 10", 10},
        et {"2 * 2 * 2 * 2 * 2", 32},
        et {"-50 + 100 + -50", 0},
        et {"5 * 2 + 10", 20},
        et {"5 + 2 * 10", 25},
        et {"20 + 2 * -10", 0},
        et {"50 / 2 * 2 + 10", 60},
        et {"2 * (5 + 10)", 30},
        et {"3 * 3 * 3 + 10", 37},
        et {"3 * (3 * 3) + 10", 37},
        et {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected);
    }
}

TEST_CASE("characterExpresssion")
{
    struct ct
    {
        std::string_view input;
        char expected;
    };

    std::array tests {
        ct {"'5'", '5'},
        ct {"'a'", 'a'},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected);
    }
}

TEST_CASE("booleanExpresssion")
{
    struct et
    {
        std::string_view input;
        bool expected;
    };

    std::array tests {
        et {"true", true},
        et {"false", false},
        et {"1 < 2", true},
        et {"1 > 2", false},
        et {"1 < 1", false},
        et {"1 > 1", false},
        et {"1 == 1", true},
        et {"1 != 1", false},
        et {"1 == 2", false},
        et {"1 != 2", true},
        et {"'a' < 'b'", true},
        et {"'a' > 'b'", false},
        et {"'a' < 'a'", false},
        et {"'a' > 'a'", false},
        et {"'a' == 'a'", true},
        et {"'a' != 'a'", false},
        et {"'a' == 'b'", false},
        et {"'a' != 'b'", true},

    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        INFO("with input: ", input);
        require_eq(evaluated, expected);
    }
}

TEST_CASE("stringExpression")
{
    auto evaluated = run(R"("Hello World!")");
    require_eq<string_type>(evaluated, "Hello World!");
}

TEST_CASE("stringConcatenation")
{
    auto evaluated = run(R"("Hello" + " " + "World!")");
    INFO("expected a string, got: ", evaluated.type_name());
    require_eq<string_type>(evaluated, "Hello World!");
}

TEST_CASE("characterExpression")
{
    auto evaluated = run(R"('H')");
    require_eq<char>(evaluated, 'H');
}

TEST_CASE("stringCharacterConcatenation")
{
    struct et
    {
        std::string_view input;
        string_type expected;
    };

    std::array tests {
        et {
            R"("Hel" + 'p')",
            "Help",
        },
        et {
            R"('H' + "ell" + 'o')",
            "Hello",
        },
        et {
            R"('H' + 'e' + 'l' + 'l' + 'o')",
            "Hello",
        },
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        INFO("with input: `", input, "` ", evaluated.as<string_type>().length(), " <-> ", expected.length());
        require_eq(evaluated, expected);
    }
}

TEST_CASE("bangOperator")
{
    struct et
    {
        std::string_view input;
        bool expected;
    };

    std::array tests {
        et {"!true", false},
        et {"!false", true},
        et {"!false", true},
        et {"!5", false},
        et {"!'a'", false},
        et {"!!true", true},
        et {"!!false", false},
        et {"!!5", true},
        et {"!!'a'", true},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected);
    }
}

TEST_CASE("ifElseExpressions")
{
    struct et
    {
        std::string_view input;
        object expected;
    };

    std::array tests {
        et {"if (true) { 10 }", {10}},
        et {"if (false) { 10 }", {}},
        et {"if (1) { 10 }", {10}},
        et {"if (1 < 2) { 10 }", {10}},
        et {"if (1 > 2) { 10 }", {}},
        et {"if (1 > 2) { 10 } else { 20 }", {20}},
        et {"if (1 < 2) { 10 } else { 20 }", {10}},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        if (expected.is<integer_type>()) {
            require_eq(evaluated, expected.as<integer_type>());
        } else {
            require_eq(evaluated, nilv);
        }
    }
}

TEST_CASE("returnStatemets")
{
    struct rt
    {
        std::string_view input;
        integer_type expected;
    };

    std::array tests {
        rt {"return 10;", 10},
        rt {"return 10; 9;", 10},
        rt {"return 2 * 5; 9;", 10},
        rt {"9; return 2 * 5; 9;", 10},
        rt {R"r(
if (10 > 1) {
    if (10 > 1) {
        return 10;
    }
    return 1;
})r",
            10},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected);
    }
}

TEST_CASE("errorHandling")
{
    struct et
    {
        std::string_view input;
        std::string expected_message;
    };

    std::array tests {
        et {
            "5 + true;",
            "type mismatch: integer + bool",
        },
        et {
            "5 + true; 5;",
            "type mismatch: integer + bool",
        },
        et {
            "-true",
            "unknown operator: -bool",
        },
        et {
            "true + false;",
            "unknown operator: bool + bool",
        },
        et {
            "5; true + false; 5",
            "unknown operator: bool + bool",
        },
        et {
            "if (10 > 1) { true + false; }",
            "unknown operator: bool + bool",
        },
        et {
            R"r(
if (10 > 1) {
if (10 > 1) {
return true + false;
}
return 1;
}
   )r",
            "unknown operator: bool + bool",
        },
        et {
            "foobar",
            "identifier not found: foobar",
        },
        et {
            R"("Hello" - "World")",
            "unknown operator: string - string",
        },
        et {
            R"({"name": "Monkey"}[fn(x) { x }];)",
            "unusable as hash key: function",
        },
    };

    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        INFO("expected an error, got ", evaluated.type_name());
        require_eq(evaluated, error {expected});
    }
}

TEST_CASE("integerLetStatements")
{
    struct lt
    {
        std::string_view input;
        integer_type expected;
    };

    std::array tests {
        lt {"let a = 5; a;", 5},
        lt {"let a = 5 * 5; a;", 25},
        lt {"let a = 5; let b = a; b;", 5},
        lt {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
    };

    for (const auto& [input, expected] : tests) {
        require_eq(run(input), expected);
    }
}

TEST_CASE("boundFunction")
{
    const auto* input = "fn(x) {x + 2; };";
    auto evaluated = run(input);
    INFO("expected a function object, got ", std::to_string(evaluated.value));
    REQUIRE(evaluated.is<bound_function>());
}

TEST_CASE("functionApplication")
{
    struct ft
    {
        std::string_view input;
        int64_t expected;
    };

    std::array tests {
        ft {"let identity = fn(x) { x; }; identity(5);", 5},
        ft {"let identity = fn(x) { return x; }; identity(5);", 5},
        ft {"let double = fn(x) { x * 2; }; double(5);", 10},
        ft {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        ft {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        ft {"fn(x) { x; }(5)", 5},
        ft {"let c = fn(x) { x + 2; }; c(2 + c(4))", 10},
    };
    for (const auto& [input, expected] : tests) {
        require_eq(run(input), expected);
    }
}

TEST_CASE("multipleEvaluationsWithSameEnvAndDestroyedSources")
{
    const auto* input1 {R"(let makeGreeter = fn(greeting) { fn(name) { greeting + " " + name + "!" } };)"};
    const auto* input2 {R"(let hello = makeGreeter("hello");)"};
    const auto* input3 {R"(hello("banana");)"};
    std::deque<std::string> inputs {input1, input2, input3};
    require_eq<string_type>(run_multi(inputs), "hello banana!");
}

TEST_CASE("builtinFunctions")
{
    struct bt
    {
        std::string_view input;
        std::variant<char, std::int64_t, std::string, error, nil_type, array> expected;
    };

    std::array tests {
        bt {R"(len(""))", 0},
        bt {R"(len("four"))", 4},
        bt {R"(len("hello world"))", 11},
        bt {R"(len(1))", error {"argument of type integer to len() is not supported"}},
        bt {R"(len("one", "two"))", error {"wrong number of arguments to len(): expected=1, got=2"}},
        bt {R"(len([1,2]))", 2},
        bt {R"(first("abc"))", 'a'},
        bt {R"(first())", error {"wrong number of arguments to first(): expected=1, got=0"}},
        bt {R"(first(1))", error {"argument of type integer to first() is not supported"}},
        bt {R"(first([1,2]))", 1},
        bt {R"(first([]))", nilv},
        bt {R"(last("abc"))", 'c'},
        bt {R"(last())", error {"wrong number of arguments to last(): expected=1, got=0"}},
        bt {R"(last(1))", error {"argument of type integer to last() is not supported"}},
        bt {R"(last([1,2]))", 2},
        bt {R"(last([]))", nilv},
        bt {R"(rest("abc"))", "bc"},
        bt {R"(rest("bc"))", "c"},
        bt {R"(rest("c"))", nilv},
        bt {R"(rest())", error {"wrong number of arguments to rest(): expected=1, got=0"}},
        bt {R"(rest(1))", error {"argument of type integer to rest() is not supported"}},
        bt {R"(rest([1,2]))", array {{2}}},
        bt {R"(rest([1]))", nilv},
        bt {R"(rest([]))", nilv},
        bt {R"(push())", error {"wrong number of arguments to push(): expected=2, got=0"}},
        bt {R"(push(1))", error {"wrong number of arguments to push(): expected=2, got=1"}},
        bt {R"(push(1, 2))", error {"argument of type integer and integer to push() are not supported"}},
        bt {R"(push([1,2], 3))", array {{1}, {2}, {3}}},
        bt {R"(push([], "abc"))", array {{"abc"}}},
        bt {R"(push("", 'a'))", "a"},
        bt {R"(push("c", 'a'))", "ca"},
    };

    for (auto test : tests) {
        auto evaluated = run(test.input);
        INFO("with input: `", test.input, "`");
        std::visit(
            overloaded {
                [&evaluated](const char val) { require_eq(evaluated, val); },
                [&evaluated](const integer_type val) { require_eq(evaluated, val); },
                [&evaluated](const error& val) { require_eq(evaluated, val); },
                [&evaluated](const std::string& val) { require_eq(evaluated, val); },
                [&evaluated](const array& val) { REQUIRE_EQ(object {val}, evaluated); },
                [&evaluated](const nil_type& /*val*/) { require_eq(evaluated, nilv); },
            },
            test.expected);
    }
}

TEST_CASE("arrayExpression")
{
    auto evaluated = run("[1, 2 * 2, 3 + 3]");
    INFO("got: " << evaluated.type_name());
    REQUIRE(evaluated.is<array>());
    auto as_arr = evaluated.as<array>();
    require_eq<integer_type>(as_arr[0], 1);
    require_eq<integer_type>(as_arr[1], 4);
    require_eq<integer_type>(as_arr[2], 6);
}

TEST_CASE("indexOperatorExpressions")
{
    struct it
    {
        std::string_view input;
        value_type expected;
    };

    std::array tests {
        it {
            "[1, 2, 3][0]",
            1,
        },
        it {
            "[1, 2, 3][1]",
            2,
        },
        it {
            "[1, 2, 3][2]",
            3,
        },
        it {
            "let i = 0; [1][i];",
            1,
        },
        it {
            "[1, 2, 3][1 + 1];",
            3,
        },
        it {
            "let myArray = [1, 2, 3]; myArray[2];",
            3,
        },
        it {
            "let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];",
            6,
        },
        it {
            "let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i]",
            2,
        },
        it {
            "[1, 2, 3][3]",
            nilv,
        },
        it {
            "[1, 2, 3][-1]",
            nilv,
        },
        it {
            R"("2"[0])",
            '2',
        },
        it {
            R"(""[0])",
            nilv,
        },
    };

    for (const auto& [input, expected] : tests) {
        auto evaluated = run(input);
        INFO("got: ", evaluated.type_name(), " with: ", std::to_string(evaluated.value));
        CHECK_EQ(object {evaluated.value}, object {expected});
    }
}

TEST_CASE("hashLiterals")
{
    auto evaluated = run(R"(let two = "two";
    {
        "one": 10 - 9,
        two: 1 + 1,
        "thr" + "ee": 6 / 2,
        4: 4,
        true: 5,
        false: 6
    })");

    INFO("expected hash, got: " << evaluated.type_name());
    REQUIRE(evaluated.is<hash>());

    struct expect
    {
        hash_key_type key;
        int64_t value;
    };

    std::array expected {
        expect {"one", 1},
        expect {"two", 2},
        expect {"three", 3},
        expect {4, 4},
        expect {true, 5},
        expect {false, 6},
    };
    const auto& as_hash = evaluated.as<hash>();
    for (const auto& [key, value] : expected) {
        REQUIRE(as_hash.contains(key));
        REQUIRE_EQ(value, std::any_cast<object>(as_hash.at(key)).as<integer_type>());
    };
}

TEST_CASE("hashIndexExpression")
{
    struct ht
    {
        std::string_view input;
        value_type expected;
    };

    std::array tests {
        ht {
            R"({"foo": 5}["foo"])",
            5,
        },
        ht {
            R"({"foo": 5}["bar"])",
            nilv,
        },
        ht {
            R"(let key = "foo"; {"foo": 5}[key])",
            5,
        },
        ht {
            R"({}["foo"])",
            nilv,
        },
        ht {
            R"({}['5'])",
            nilv,
        },
        ht {
            R"({5: 5}[5])",
            5,
        },
        ht {
            R"({true: 5}[true])",
            5,
        },
        ht {
            R"({false: 5}[false])",
            5,
        },
        ht {
            R"({'a': 5}['a'])",
            5,
        },
    };
    for (const auto& [input, expected] : tests) {
        auto evaluated = run(input);
        CHECK_FALSE(evaluated.is<error>());
        CHECK_EQ(evaluated, object {expected});
    }
}

TEST_SUITE_END();

// NOLINTEND(*)
}  // namespace
