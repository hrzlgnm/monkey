#pragma once
#include <utility>
#include <vector>

template<typename T>
class chungus
{
  public:
    static void track(T* obj) { get_allocation_list().push_back(obj); }

    static void cleanup()
    {
        for (T* obj : get_allocation_list()) {
            delete obj;  // Properly call destructor
        }
        get_allocation_list().clear();
    }

  private:
    static auto get_allocation_list() -> std::vector<T*>&
    {
        static std::vector<T*> allocations;
        static const bool registered = []()
        {
            return std::atexit(cleanup);  // Register cleanup at exit
        }();
        (void)registered;  // Avoid unused variable warning
        return allocations;
    }
};

template<typename T, typename... Args>
auto make(Args&&... args) -> T*
{
    T* p = new T(std::forward<Args>(args)...);
    chungus<T>::track(p);
    return p;
}
