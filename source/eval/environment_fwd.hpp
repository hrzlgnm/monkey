#pragma once

#include <memory>

struct environment;
using environment_ptr = std::shared_ptr<environment>;

auto debug_env(const environment_ptr& env) -> void;
