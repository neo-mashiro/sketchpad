#pragma once

/* this file may contain the following at the project's level (root level)

   > detection and pre-check of platform, compiler, version, etc.
   > switch on build/compilation mode, context and backends
   > global macros definition and typedefs
   > global constexpr variables and functions
   > global enum classes, type/namespace aliases, alias templates
   > global function wrappers? template wrapper classes? depends on usage and purpose
   > ......
   > global utility or helper stuff should go to "utils/ext.h" instead
*/

// require Windows x86 (32 bit) or x64 (64 bit)
#ifndef _WIN32
    // android is based on the linux kernel so it also has __linux__ defined
    #if defined(__ANDROID__) || defined(__linux__)
        #error "Linux platform is not supported..."
    #elif defined(__APPLE__) || defined(__MACH__)  // iOS device or MacOS
        #error "Apple platform is not supported..."
    #endif

    #error "Unsupported platform, compilation aborted..."
#endif

// require C++17 language and library features
#if !defined(_MSC_VER) && __cplusplus < 201703L
    #error "C++17 or above required, compilation aborted..."
#endif

// require VS >= 2019.16.2 or GCC >= 8.1
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) > 800
    #if defined(__i386__) || defined(__x86_64__)
        #define SP_DBG_BREAK() __asm__ volatile("int $0x03")
    #elif defined(__thumb__)
        #define SP_DBG_BREAK() __asm__ volatile(".inst 0xde01")
    #elif defined(__arm__) && !defined(__thumb__)
        #define SP_DBG_BREAK() __asm__ volatile(".inst 0xe7f001f0");
    #endif
#elif defined(_MSC_VER) && _MSC_VER >= 1922
    #define SP_DBG_BREAK() __debugbreak()
#else
    #error "Unsupported compiler, compilation aborted..."
#endif

// report memory leaks on the new operator (copy this to every target cpp file)
#if 0 && (defined(_DEBUG) || defined(DEBUG))
    #define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
    #define new DBG_NEW 
#endif

// translate macros into `inline constexpr` variables (external linkage) which
// are globally visible to all translation units w/o violating the ODR rule so
// as to avoid lots of `#ifdef` blocks from splattering all over our code base

#if defined(_DEBUG) || defined(DEBUG)
    inline constexpr bool debug_mode = true;
#else
    inline constexpr bool debug_mode = false;
#endif

#ifdef __FREEGLUT__
    inline constexpr bool _freeglut = true;
#else
    inline constexpr bool _freeglut = false;
#endif

#include <memory>

constexpr auto __APP__ = "sketchpad";
constexpr auto __VER__ = 202109U;

template<typename T>
inline constexpr bool const_false = false;

template<typename T> using asset_ref = std::shared_ptr<T>;
template<typename T> using asset_tmp = std::unique_ptr<T>;

template<typename T, typename... Args>
constexpr asset_ref<T> MakeAsset(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
constexpr asset_tmp<T> WrapAsset(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}