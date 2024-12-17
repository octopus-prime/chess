#pragma once

#include "square.hpp"

class move_t {
public:
    enum type_t : std::uint8_t {
        KING,
        CASTLE_SHORT,
        CASTLE_LONG,
        QUEEN,
        ROOK,
        BISHOP,
        KNIGHT,
        PAWN,
        PROMOTE_QUEEN,
        PROMOTE_ROOK,
        PROMOTE_BISHOP,
        PROMOTE_KNIGHT,
        DOUBLE_PUSH,
        EN_PASSANT
    };

private:
    type_t type_;
    square from_;
    square to_;

public:
    constexpr move_t(type_t type = KING, square from = a1, square to = a1) noexcept
        : type_{type}, from_{from}, to_{to} {}
    constexpr type_t type() const noexcept { return type_; }
    constexpr square from() const noexcept { return from_; }
    constexpr square to() const noexcept { return to_; }
    constexpr bool operator==(const move_t& other) const noexcept = default;
};

template <>
struct std::formatter<move_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(move_t move, std::format_context& ctx) const {
        switch (move.type())
        {
        case move_t::KING:
            return std::format_to(ctx.out(), "K{}{}", move.from(), move.to());
        case move_t::CASTLE_SHORT:
            return std::format_to(ctx.out(), "O-O");
        case move_t::CASTLE_LONG:
            return std::format_to(ctx.out(), "O-O-O");
        case move_t::KNIGHT:
            return std::format_to(ctx.out(), "N{}{}", move.from(), move.to());
        case move_t::QUEEN:
            return std::format_to(ctx.out(), "Q{}{}", move.from(), move.to());
        case move_t::ROOK:
            return std::format_to(ctx.out(), "R{}{}", move.from(), move.to());
        case move_t::BISHOP:
            return std::format_to(ctx.out(), "B{}{}", move.from(), move.to());
        case move_t::PAWN:
        case move_t::DOUBLE_PUSH:
            return std::format_to(ctx.out(), "P{}{}", move.from(), move.to());
        case move_t::PROMOTE_QUEEN:
            return std::format_to(ctx.out(), "P{}{}Q", move.from(), move.to());
        case move_t::PROMOTE_ROOK:
            return std::format_to(ctx.out(), "P{}{}R", move.from(), move.to());
        case move_t::PROMOTE_BISHOP:
            return std::format_to(ctx.out(), "P{}{}B", move.from(), move.to());
        case move_t::PROMOTE_KNIGHT:
            return std::format_to(ctx.out(), "P{}{}N", move.from(), move.to());
        case move_t::EN_PASSANT:
            return std::format_to(ctx.out(), "P{}{}ep", move.from(), move.to());
        }
    }
};
