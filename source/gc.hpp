#pragma once

#include <concepts>
#include <cstdlib>
#include <utility>
#include <vector>

#include <ast/expression.hpp>
#include <object/object.hpp>

template<typename T>
class gc
{
  public:
    static void track(T* obj) { get_allocation_list().push_back(obj); }

  private:
    static void cleanup()
    {
        for (T* obj : get_allocation_list()) {
            delete obj;
        }
    }

    static auto get_allocation_list() -> std::vector<T*>&
    {
        static std::vector<T*> allocations;
        static const bool registered = []() { return std::atexit(cleanup); }();
        (void)registered;
        return allocations;
    }
};

template<typename O, typename... Args>
    requires std::derived_from<O, struct object>
auto make(Args&&... args) -> O*
{
    O* p = new O(std::forward<Args>(args)...);
    gc<object>::track(p);
    return p;
}

template<typename E, typename... Args>
    requires std::derived_from<E, struct expression>
auto make(Args&&... args) -> E*
{
    E* p = new E(std::forward<Args>(args)...);
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
