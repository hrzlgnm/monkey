#pragma once

#include <ast/array_literal.hpp>
#include <ast/assign_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean_literal.hpp>
#include <ast/call_expression.hpp>
#include <ast/decimal_literal.hpp>
#include <ast/expression.hpp>
#include <ast/function_literal.hpp>
#include <ast/hash_literal.hpp>
#include <ast/identifier.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/null_literal.hpp>
#include <ast/program.hpp>
#include <ast/statements.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>

#include "expression.hpp"

struct visitor
{
    visitor(const visitor&) = delete;
    visitor(visitor&&) = delete;
    auto operator=(const visitor&) -> visitor& = delete;
    auto operator=(visitor&&) -> visitor& = delete;
    visitor() = default;
    virtual ~visitor() = default;

    virtual void visit(const array_literal& expr) = 0;
    virtual void visit(const assign_expression& expr) = 0;
    virtual void visit(const binary_expression& expr) = 0;
    virtual void visit(const block_statement& expr) = 0;
    virtual void visit(const boolean_literal& expr) = 0;
    virtual void visit(const break_statement& expr) = 0;
    virtual void visit(const call_expression& expr) = 0;
    virtual void visit(const continue_statement& expr) = 0;
    virtual void visit(const decimal_literal& expr) = 0;
    virtual void visit(const expression_statement& expr) = 0;
    virtual void visit(const function_literal& expr) = 0;
    virtual void visit(const hash_literal& expr) = 0;
    virtual void visit(const identifier& expr) = 0;
    virtual void visit(const if_expression& expr) = 0;
    virtual void visit(const index_expression& expr) = 0;
    virtual void visit(const integer_literal& expr) = 0;
    virtual void visit(const let_statement& expr) = 0;
    virtual void visit(const null_literal& expr) = 0;
    virtual void visit(const program& expr) = 0;
    virtual void visit(const return_statement& expr) = 0;
    virtual void visit(const string_literal& expr) = 0;
    virtual void visit(const unary_expression& expr) = 0;
    virtual void visit(const while_statement& expr) = 0;
};
