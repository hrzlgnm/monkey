#pragma once

#include "code.hpp"
#include "object.hpp"
#include "program.hpp"

using constants = std::vector<object>;

struct bytecode
{
    instructions code;
    constants consts;
};

struct compiler
{
    instructions cod;
    constants consts;
    auto compile(const program_ptr& program) -> void;
    auto code() const -> bytecode;
};
