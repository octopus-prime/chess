#pragma once

#include "square.hpp"
#include "piece.hpp"

struct move_t {
    constexpr move_t() noexcept : from_{NO_SQUARE}, to_{NO_SQUARE}, promotion_{NO_TYPE} {
    }

    constexpr move_t(square from, square to, type_e promotion = NO_TYPE) noexcept : from_{from}, to_{to}, promotion_{promotion} {
    }

    constexpr move_t(std::string_view string) noexcept : from_{string.substr(0, 2)}, to_{string.substr(2, 2)} {
        string.remove_prefix(4);
        promotion_ = operator""_t(string.data(), string.size());
    }

    constexpr bool operator==(const move_t& other) const noexcept = default;

    constexpr square from() const noexcept {
        return from_;
    }

    constexpr square to() const noexcept {
        return to_;
    }

    constexpr type_e promotion() const noexcept {
        return promotion_;
    }

private:
    square from_;
    square to_;
    type_e promotion_;
};

inline constexpr move_t operator""_m(const char* str, size_t len) noexcept {
    return std::string_view{str, len};
}


template <>
struct std::formatter<move_t> {
    constexpr auto parse(std::format_parse_context& ctx){
        return ctx.begin();
    }

    auto format(move_t move, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}{}{}", move.from(), move.to(), move.promotion());
    }
};

template<typename T>
struct std::formatter<std::span<T>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(std::span<T> span, std::format_context& ctx) const {
        for (size_t i = 0; i < span.size(); ++i) {
            if (i > 0) std::format_to(ctx.out(), "{}", ' ');
            std::format_to(ctx.out(), "{}", span[i]);
        }
        return ctx.out();
    }
};
