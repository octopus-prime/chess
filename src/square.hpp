#pragma once

#include "common.hpp"

enum rank_e : int8_t {
    R1, R2, R3, R4, R5, R6, R7, R8
};
constexpr inline size_t RANK_MAX = 8;

enum file_e : int8_t { 
    FA, FB, FC, FD, FE, FF, FG, FH
};
constexpr inline size_t FILE_MAX = 8;

enum square_e : int8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};
constexpr inline size_t SQUARE_MAX = 64;

struct square {
    constexpr square() noexcept
        : value{} {
    }

    constexpr square(square_e value) noexcept
        : value{value} {
    }

    constexpr square(file_e file, rank_e rank) noexcept
        : square{static_cast<square_e>(FILE_MAX * rank + file)} {
    }

    constexpr square(std::string_view string) noexcept
        : square{static_cast<file_e>(string[0] - 'a'), static_cast<rank_e>(string[1] - '1')} {
    }

    constexpr rank_e rank() const noexcept {
        return static_cast<rank_e>(value / FILE_MAX);
    }

    constexpr file_e file() const noexcept {
        return static_cast<file_e>(value % FILE_MAX);
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

    constexpr square operator+(std::integral auto value) const noexcept {
        return static_cast<square_e>(this->value + value);
    }

    constexpr square operator-(std::integral auto value) const noexcept {
        return static_cast<square_e>(this->value - value);
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
