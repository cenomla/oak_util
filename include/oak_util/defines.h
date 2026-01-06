#pragma once

#ifndef NDEBUG
#if defined(_MSC_VER) && !defined(__clang__)
#define OAK_UNREACHABLE(str) do { assert(str && false); __assume(false); } while (0)
#else
#define OAK_UNREACHABLE(str) do { assert(str && false); __builtin_unreachable(); } while (0)
#endif
#else
#if defined(_MSC_VER) && !defined(__clang__)
#define OAK_UNREACHABLE(str) __assume(false)
#else
#define OAK_UNREACHABLE(str) __builtin_unreachable()
#endif
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define OAK_FINLINE static __forceinline
#else
#define OAK_FINLINE __attribute__((always_inline)) static inline
#endif

#ifdef _MSC_VER

#ifdef OAK_UTIL_DYNAMIC_LIB

#ifdef OAK_UTIL_EXPORT_SYMBOLS
#define OAK_UTIL_API __declspec(dllexport)
#else
#define OAK_UTIL_API __declspec(dllimport)
#endif // OAK_UTIL_EXPORT_SYMBOLS

#else

#define OAK_UTIL_API

#endif // OAK_UTIL_DYNAMIC_LIB

#else

#define OAK_UTIL_API __attribute__((visibility("default")))

#endif // _MSC_VER

#if !(defined(__GNUG__) || defined(_MSC_VER))
// Some stdlib implementations treat uint64_t and size_t as different types so we override in that case,
// if we we're to always enable the override then we'd get multiple function definition errors (lol, treat two of the same types as different types but only on some platforms, thanks c++!)
#define USIZE_OVERRIDE_NEEDED
#endif

#define MACRO_CAT_IMPL(x, y) x##y
#define MACRO_CAT(x, y) MACRO_CAT_IMPL(x, y)

#ifndef __OSIG_REFLECT_MACRO__
#define __OSIG_REFLECT_MACRO__

#ifdef __OSIG__
#define _reflect(...) __attribute__((annotate("reflect;" #__VA_ARGS__)))
#else
#define _reflect(...)
#endif

#endif //__OSIG_REFLECT_MACRO__

#ifdef _MSC_VER
#define OAK_DBG_NO_OPT
#define OAK_DBG_NO_OPT_FILE _Pragma("optimize(\"\", off)")
#else
#define OAK_DBG_NO_OPT __attribute__((optimize("O0")))
#define OAK_DBG_NO_OPT_FILE _Pragma("GCC push_options") _Pragma("GCC optimize(\"O0\")")
#endif

