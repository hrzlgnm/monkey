#pragma once

#include <ast/expression.hpp>
#include <ast/program.hpp>
#include <ast/visitor.hpp>
#include <object/object.hpp>

#include "environment.hpp"

struct evaluator final : visitor
{
    explicit evaluator(environment* existing_env = nullptr);
    auto evaluate(const program* prgrm) -> const object*;

  protected:
    void visit(const array_literal& expr) final;
    void visit(const assign_expression& expr) final;
    void visit(const binary_expression& expr) final;
    void visit(const block_statement& expr) final;
    void visit(const boolean_literal& expr) final;
    void visit(const break_statement& expr) final;
    void visit(const call_expression& expr) final;
    void visit(const continue_statement& expr) final;
    void visit(const decimal_literal& expr) final;
    void visit(const expression_statement& expr) final;
    void visit(const function_literal& expr) final;
    void visit(const hash_literal& expr) final;
    void visit(const identifier& expr) final;
    void visit(const if_expression& expr) final;
    void visit(const index_expression& expr) final;
    void visit(const integer_literal& expr) final;
    void visit(const let_statement& expr) final;
    void visit(const program& expr) final;
    void visit(const return_statement& expr) final;
    void visit(const string_literal& expr) final;
    void visit(const unary_expression& expr) final;
    void visit(const while_statement& expr) final;

    void visit(const builtin_function& expr) final {}

  private:
    void apply_function(const object* function_or_builtin, array_object::value_type&& args);
    auto evaluate_expressions(const expressions& exprs) -> array_object::value_type;
    const object* m_result {};
    environment* m_env;
};
