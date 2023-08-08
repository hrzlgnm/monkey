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

constexpr auto bits_in_byte = 8U;
constexpr auto byte_mask = 0xFFU;

auto read_uint16_big_endian(const std::vector<uint8_t>& bytes, size_t offset) -> uint16_t
{
    if (offset + 2 > bytes.size()) {
        throw std::out_of_range("Offset is out of bounds");
    }

    auto result = static_cast<uint16_t>((bytes[offset]) << bits_in_byte);
    result |= bytes[offset + 1];

    return result;
}

void write_uint16_big_endian(std::vector<uint8_t>& bytes, size_t offset, uint16_t value)
{
    if (offset + 2 > bytes.size()) {
        bytes.resize(offset + 2);
    }

    bytes[offset] = static_cast<uint8_t>(value >> bits_in_byte);
    bytes[offset + 1] = static_cast<uint8_t>(value & byte_mask);
}
