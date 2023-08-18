#include <iostream>

#include "util.hpp"

#include "environment.hpp"
#include "object.hpp"

auto debug_env(const environment_ptr& env) -> void
{
    for (const auto& [k, v] : env->store) {
        std::cout << "[" << k << "] = " << std::to_string(v.value) << "\n";
    }
}
