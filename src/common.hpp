#pragma once

#include <immintrin.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <ranges>
#include <span>
#include <fstream>
#include <bit>
#include <utility>
#include <string_view>
#include <numeric>
#include <format>
#include <functional>
#include <regex>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>

using namespace std::literals;
using Clock = std::chrono::high_resolution_clock;

template <typename T, typename U, std::size_t I>
    requires(sizeof(T) % sizeof(U) == 0)
auto span_cast(const std::span<U, I> span) noexcept {
    constexpr auto O = I / (sizeof(T) / sizeof(U));
    return std::span<T, O>{reinterpret_cast<T*>(span.data()), O};
}

template <typename T>
    requires std::is_enum_v<T>
constexpr auto enum_range(T first, T last) noexcept {
    auto f = std::to_underlying(first);
    auto l = std::to_underlying(last);
    return std::views::iota(f, l + 1) | std::views::transform([](auto e) { return static_cast<T>(e); });
}
