#pragma once

// helper type for std::visit
template<typename... T>
struct overloaded : T...
{
    using T::operator()...;
};
template<class... T>
overloaded(T...) -> overloaded<T...>;
