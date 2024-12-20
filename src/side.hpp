#pragma once

#include "common.hpp"

enum side_e : int8_t {
     WHITE, BLACK 
};
constexpr inline size_t SIDE_MAX = 2;

constexpr side_e operator!(side_e side) noexcept { return static_cast<side_e>(1 - side); }
constexpr side_e operator~(side_e side) noexcept { return static_cast<side_e>(1 - side); }

// struct side {
//     constexpr side(side_e value) noexcept
//         : value{value} {
//     }

//     constexpr operator side_e() const noexcept {
//         return value;
//     }

//     constexpr side_e operator~() const noexcept {
//         return static_cast<side_e>(1 - value);
//     }

//    private:
//     side_e value;
// };
