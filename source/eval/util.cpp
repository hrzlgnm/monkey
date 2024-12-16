
#include "util.hpp"

#include <gc.hpp>

#include "object.hpp"

namespace
{
template<class T, class U>
concept Derived = std::is_base_of_v<U, T>;

template<Derived<object> SequenceObject>
auto multiply_sequence(const SequenceObject* source, integer_object::value_type count) -> SequenceObject*
{
    typename SequenceObject::value_type target;
    for (integer_object::value_type i = 0; i < count; i++) {
        std::copy(source->value.cbegin(), source->value.cend(), std::back_inserter(target));
    }
    return make<SequenceObject>(std::move(target));
}

template<typename T>
auto try_mul(const object* lhs, const object* rhs) -> const object*
{
    using enum object::object_type;
    const auto seq_type = T {}.type();
    if (lhs->is(integer) && rhs->is(seq_type)) {
        return multiply_sequence(rhs->as<T>(), lhs->as<integer_object>()->value);
    }
    if (lhs->is(seq_type) && rhs->is(integer)) {
        return multiply_sequence(lhs->as<T>(), rhs->as<integer_object>()->value);
    }
    return nullptr;
}

}  // namespace

auto evaluate_sequence_mul(const object* lhs, const object* rhs) -> const object*
{
    if (const auto* result = try_mul<array_object>(lhs, rhs)) {
        return result;
    }
    if (const auto* result = try_mul<string_object>(lhs, rhs)) {
        return result;
    }
    return nullptr;
}
