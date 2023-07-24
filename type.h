#pragma once

#include <string_view>

template <typename T>
constexpr std::string_view type() {
  constexpr std::string_view pretty_function = __PRETTY_FUNCTION__;
  constexpr std::size_t begin = pretty_function.find_first_of('=') + 2;
  constexpr std::size_t end = std::min(pretty_function.find_first_of('<'),
                                       pretty_function.find_last_of(']'));
  return pretty_function.substr(begin, end - begin);
}

template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

#if __cplusplus < 202002L
template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
#endif

#ifdef NDEBUG
#define LOG(x, ...)
#else
#include <cstdio>
#define LOG(x, ...)                         \
  printf("[%s:%d]:\t" x "\n", __FUNCTION__, \
         __LINE__ __VA_OPT__(, )            \
             __VA_ARGS__)  // [note]: __VA_POT__(,) decide provide a comma
                           // according to the number of arguments passed to the
                           // MACRO
#endif