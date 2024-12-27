#pragma once

#include <unordered_map>

#include <ast/expression.hpp>
#include <ast/program.hpp>
#include <ast/visitor.hpp>
#include <compiler/symbol_table.hpp>
#include <eval/environment.hpp>
#include <lexer/token_type.hpp>
#include <object/object.hpp>

struct analyzer final : visitor
{
    explicit analyzer(symbol_table* symbols);
    void analyze(const program* prgrm) noexcept(false);

    void visit(const array_literal& expr) final;
    void visit(const assign_expression& expr) final;
    void visit(const binary_expression& expr) final;
    void visit(const block_statement& expr) final;
    void visit(const expression_statement& expr) final;
    void visit(const function_literal& expr) final;
    void visit(const hash_literal& expr) final;
    void visit(const identifier& expr) final;
    void visit(const if_expression& expr) final;
    void visit(const index_expression& expr) final;
    void visit(const let_statement& expr) final;
    void visit(const program& expr) final;
    void visit(const return_statement& expr) final;
    void visit(const unary_expression& expr) final;
    void visit(const while_statement& expr) final;
    void visit(const break_statement& expr) final;
    void visit(const call_expression& expr) final;
    void visit(const continue_statement& expr) final;
    void visit(const boolean_literal& /* expr */) final;
    void visit(const decimal_literal& /* expr */) final;
    void visit(const integer_literal& /* expr */) final;
    void visit(const string_literal& /* expr */) final;

  private:
    void see(object::object_type type);
    void see(const std::string& identifier);
    void see(const std::string& identifier, object::object_type type);
    symbol_table* m_symbols;
    std::optional<object::object_type> m_last_type;
    std::unordered_map<std::string, object::object_type> m_symbol_types;
};

void analyze_program(const program* program,
                     symbol_table* existing_symbols,
                     const environment* existing_env) noexcept(false);
