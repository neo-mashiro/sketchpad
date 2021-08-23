#pragma once

#include <memory>
#include <string_view>

template<typename T>
using asset_ref = std::shared_ptr<T>;

template<typename T, typename... Args>
constexpr asset_ref<T> LoadAsset(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// MSVC intrinsic type name deduction
template<typename T>
constexpr auto type_name() noexcept {
    std::string_view type = __FUNCSIG__;
    type.remove_prefix(std::string_view("auto __cdecl type_name<").size());
    type.remove_suffix(std::string_view(">(void) noexcept").size());
    return type;
}

template<typename T>
inline constexpr bool const_false = false;