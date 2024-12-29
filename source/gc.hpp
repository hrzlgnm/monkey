#pragma once

#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <utility>
#include <vector>

template<typename T>
class gc
{
    using store = std::vector<T*>;

  public:
    static void track(T* obj) { get_store().push_back(obj); }

  private:
    static void cleanup()
    {
        for (T* obj : get_store()) {
            delete obj;
        }
    }

    static auto get_store() -> store&
    {
        static store allocations;
        static const bool registered = []()
        {
            if constexpr (std::is_same_v<T, struct object>) {
                constexpr auto object_reserve = static_cast<const size_t>(64 * 1024 * 1024);
                allocations.reserve(object_reserve);
            } else {
                constexpr auto reserve = static_cast<const size_t>(128);
                allocations.reserve(reserve);
            }
            return std::atexit(cleanup);
        }();
        (void)registered;
        return allocations;
    }
};

template<typename T, typename... Args>
    requires std::derived_from<T, struct object>
auto make(Args&&... args) -> T*
{
    T* p = new T(std::forward<Args>(args)...);
    gc<object>::track(p);
    return p;
}

template<typename T, typename... Args>
    requires std::derived_from<T, struct expression>
auto make(Args&&... args) -> T*
{
    T* p = new T(std::forward<Args>(args)...);
    gc<expression>::track(p);
    return p;
}

template<typename T, typename... Args>
auto make(Args&&... args) -> T*
{
    T* p = new T(std::forward<Args>(args)...);
    gc<T>::track(p);
    return p;
}
