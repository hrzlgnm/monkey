#pragma once

#include <concepts>
#include <cstdlib>
#include <utility>
#include <vector>

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
