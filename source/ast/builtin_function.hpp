#pragma once

#include <functional>
#include <string>
#include <vector>

#include <object/object.hpp>

#include "expression.hpp"

struct builtin_function final : expression
{
    builtin_function(std::string name,
                     std::vector<std::string> params,
                     std::function<const object*(std::vector<const object*>&& arguments)> bod);

    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
    auto compile(compiler& comp) const -> void override;

    static auto builtins() -> const std::vector<const builtin_function*>&;

    std::string name;
    std::vector<std::string> parameters;
    std::function<const object*(array_object::value_type&& arguments)> body;
};
