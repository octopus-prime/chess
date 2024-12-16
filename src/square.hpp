#pragma once

#include "common.hpp"

enum rank_e : int8_t {
    r_1, r_2, r_3, r_4, r_5, r_6, r_7, r_8
};
constexpr inline size_t rank_max = 8;

enum file_e : int8_t { 
    f_a, f_b, f_c, f_d, f_e, f_f, f_g, f_h
};
constexpr inline size_t file_max = 8;

enum square_e : int8_t {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};
constexpr inline size_t square_max = 64;

struct square {
    constexpr square(square_e value) noexcept
        : value{value} {
    }

    constexpr square(file_e file, rank_e rank) noexcept
        : square{static_cast<square_e>(file_max * rank + file)} {
    }

    constexpr square(std::string_view string) noexcept
        : square{static_cast<file_e>(string[0] - 'a'), static_cast<rank_e>(string[1] - '1')} {
    }

    constexpr rank_e rank() const noexcept {
        return static_cast<rank_e>(value / file_max);
    }

    constexpr file_e file() const noexcept {
        return static_cast<file_e>(value % file_max);
    }

    constexpr operator square_e() const noexcept {
        return value;
    }

    constexpr void operator++() noexcept {
        value = static_cast<square_e>(value + 1);
    }

    constexpr void operator+=(std::integral auto value) noexcept {
        this->value = static_cast<square_e>(this->value + value);
    }

   private:
    square_e value;
};

constexpr square operator""_s(const char* data, size_t length) noexcept {
  return std::string_view{data, length};
}

template <>
struct std::formatter<square> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(square sq, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{:c}{:c}", 'a' + sq.file(), '1' + sq.rank());
    }
};
