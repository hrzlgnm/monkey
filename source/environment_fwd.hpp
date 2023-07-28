#pragma once

#include <memory>

struct environment;
using environment_ptr = std::shared_ptr<environment>;
using weak_environment_ptr = std::weak_ptr<environment>;
