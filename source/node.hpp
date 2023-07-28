#pragma once

#include <memory>
#include <string>
#include <string_view>

struct environment;
using environment_ptr = std::shared_ptr<environment>;

struct object;
struct node
{
    node() = default;
    node(const node&) = delete;
    node(node&&) = delete;
    auto operator=(const node&) -> node& = delete;
    auto operator=(node&&) -> node& = delete;
    virtual ~node() = default;
    virtual auto token_literal() const -> std::string_view = 0;
    virtual auto string() const -> std::string = 0;
    virtual auto eval(environment_ptr env) const -> object = 0;
};
using node_ptr = std::shared_ptr<node>;
