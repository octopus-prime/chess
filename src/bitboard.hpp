#pragma once

#include "square.hpp"

struct bitboard /* : std::ranges::view_interface<bitboard_t> */ {
    struct iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type = square;
        using difference_type = std::ptrdiff_t;

        constexpr iterator() noexcept = default;

        constexpr iterator(uint64_t start) noexcept
            : value{start} {}

        constexpr iterator &operator++() noexcept {
            return *this;
        }

        constexpr iterator operator++(int) noexcept {
            return *this;
        }

        constexpr bool operator==(std::default_sentinel_t) const noexcept {
            return value == 0ull;
        };

        constexpr square operator*() const noexcept {
            auto x = std::countr_zero(value);
            value ^= 1ull << x;
            return static_cast<square_e>(x);
        }

       private:
        mutable uint64_t value;
    };

    constexpr bitboard() noexcept
        : value{0ull} {}

    constexpr bitboard(uint64_t v) noexcept
        : value{v} {}

    constexpr bitboard(square s) noexcept
        : bitboard{1ull << s} {}

    constexpr bitboard(std::string_view string) noexcept
        : bitboard{std::ranges::fold_left(string | std::views::chunk(2), 0ull, [](auto bb, auto &&chunk) {
              return bb | bitboard{square{std::string_view{chunk.data(), chunk.size()}}};
          })} {}

    constexpr operator uint64_t() const noexcept {
        return value;
    }

    constexpr square front() const noexcept {
        return static_cast<square_e>(std::countr_zero(value));
    }

    constexpr square back() const noexcept {
        return static_cast<square_e>(63 - std::countl_zero(value));
    }

    constexpr bool empty() const noexcept {
        return value == 0ull;
    }

    constexpr size_t size() const noexcept {
        return std::popcount(value);
    }

    constexpr bool operator[](square s) const noexcept {
        return value & (1ull << s);
    }

    constexpr iterator begin() const noexcept {
        return {value};
    }

    constexpr std::default_sentinel_t end() const noexcept {
        return std::default_sentinel;
    }

    constexpr void operator|=(bitboard squares) noexcept {
        value |= squares;
    }

    constexpr void operator&=(bitboard squares) noexcept {
        value &= squares;
    }

    constexpr void operator<<=(std::integral auto shift) noexcept {
        value <<= shift;
    }

    constexpr void operator>>=(std::integral auto shift) noexcept {
        value >>= shift;
    }

   private:
    uint64_t value;
};

constexpr bitboard operator""_b(const char *data, size_t length) {
    return std::string_view{data, length};
}

constexpr bitboard operator""_f(const char *data, size_t length) noexcept {
    constexpr auto mask = "a1a2a3a4a5a6a7a8"_b;
    const auto view = std::string_view{data, length};
    return std::transform_reduce(view.begin(), view.end(), 0ull, std::bit_or{}, [mask](char ch) { return mask << (ch - 'a'); });
}

constexpr bitboard operator""_r(const char *data, size_t length) noexcept {
    constexpr auto mask = "a1b1c1d1e1f1g1h1"_b;
    const auto view = std::string_view{data, length};
    return std::transform_reduce(view.begin(), view.end(), 0ull, std::bit_or{}, [mask](char ch) { return mask << (ch - '1') * 8; });
}

template <>
struct std::formatter<bitboard> {
    constexpr auto parse(std::format_parse_context& ctx){
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}') {
            if (*pos == 'b' || *pos == 'B')
                is_board = true;
            ++pos;
        }
        return pos;
    }

    auto format(bitboard bb, std::format_context& ctx) const {
        if (is_board) {
            char buffer[64];
            std::format_to(buffer, "{:064b}", (std::uint64_t) bb);
            for (auto row : buffer | std::views::chunk(8))
                std::format_to(ctx.out(), "{}{}{}{}{}{}{}{}\n", row[7], row[6], row[5], row[4], row[3], row[2], row[1], row[0]);
        } else {
            for (auto s : bb)
                std::format_to(ctx.out(), "{}", s);
        }
        return ctx.out();
    }

private:
    bool is_board{ false };
};
