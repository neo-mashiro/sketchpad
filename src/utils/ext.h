#pragma once

#include <algorithm>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <glm/gtc/type_ptr.hpp>

namespace utils {

    // function signature in a clean concise format
    inline const char* func_sig() noexcept {
        #ifdef _MSC_VER
            return __FUNCSIG__;
        #elif defined(__GNUC__)
            return __PRETTY_FUNCTION__;
        #endif
    }

    // template type name deduction in human-readable format (magic numbers subject to change)
    template<typename T>
    constexpr auto type_name() noexcept {
        #ifdef _MSC_VER
            std::string_view p = __FUNCSIG__;
            return std::string_view(p.data() + 84, p.size() - 84 - 7);
        #elif defined(__GNUC__)
            std::string_view p = __PRETTY_FUNCTION__;
            return std::string_view(p.data() + 49, p.find(';', 49) - 49);
        #endif
    }

    // retrieve const void* data pointers (this is a better match for basic scalar types)
    template<typename T>
    const T* val_ptr(const T& val_ref) {
        return &val_ref;
    }

    // retrieve const void* data pointers (this is the exact match for glm vec/mat types)
    template <typename... Args>
    auto val_ptr(Args&&... args) -> decltype(glm::value_ptr(std::forward<Args>(args)...)) {
        return glm::value_ptr(std::forward<Args>(args)...);
    }

    // convert any enum class to its underlying integral type
    template<typename EnumClass>
    constexpr auto to_integral(EnumClass e) {
        using T = std::underlying_type_t<EnumClass>;
        return static_cast<T>(e);
    }

    // range semantic wrappers for STL containers (will have the ranges library in C++20)
    namespace ranges {
        template<typename Container, typename Func>
        Func for_each(Container& range, Func function) {
            return std::for_each(begin(range), end(range), function);
        }

        template<typename Container, typename T>
        typename Container::const_iterator find(const Container& c, const T& value) {
            return std::find(c.begin(), c.end(), value);
        }

        template<typename Container, typename Pred>
        typename Container::const_iterator find_if(const Container& c, Pred predicate) {
            return std::find_if(c.begin(), c.end(), predicate);
        }

        template<typename T> using iter_diff = typename std::iterator_traits<T>::difference_type;

        template<typename Container, typename Pred>
        iter_diff<typename Container::const_iterator> count_if(const Container& c, Pred predicate) {
            return std::count_if(c.begin(), c.end(), predicate);
        }

        template<typename Container, typename T>
        constexpr void fill(const Container& c, const T& value) {
            std::fill(c.begin(), c.end(), value);
        }
    }

}