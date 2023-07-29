#include "util.hpp"

auto debug_env(const environment_ptr& env) -> void
{
    for (const auto& [k, v] : env->store) {
        std::cout << "k: " << k << " = " << std::to_string(v) << "\n";
    }
}
