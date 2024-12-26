#pragma once

#include <functional>
#include <string>
#include <vector>

#include <object/object.hpp>

struct builtin final
{
    builtin(std::string name,
            std::vector<std::string> params,
            std::function<const object*(std::vector<const object*>&& arguments)> bod);

    static auto builtins() -> const std::vector<const builtin*>&;

    std::string name;
    std::vector<std::string> parameters;
    std::function<const object*(array_object::value_type&& arguments)> body;
};
