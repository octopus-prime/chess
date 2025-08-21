#pragma once

#include <nnue/common.hpp>
#include "../side.hpp"
#include "../square.hpp"
#include "../piece.hpp"

namespace nnue {

enum {
    PS_NONE     = 0,
    PS_W_PAWN   = 0,
    PS_B_PAWN   = 1 * SQUARE_MAX,
    PS_W_KNIGHT = 2 * SQUARE_MAX,
    PS_B_KNIGHT = 3 * SQUARE_MAX,
    PS_W_BISHOP = 4 * SQUARE_MAX,
    PS_B_BISHOP = 5 * SQUARE_MAX,
    PS_W_ROOK   = 6 * SQUARE_MAX,
    PS_B_ROOK   = 7 * SQUARE_MAX,
    PS_W_QUEEN  = 8 * SQUARE_MAX,
    PS_B_QUEEN  = 9 * SQUARE_MAX,
    PS_KING     = 10 * SQUARE_MAX,
    PS_NB       = 11 * SQUARE_MAX
};

constexpr std::uint16_t PieceSquareIndex[SIDE_MAX][PIECE_MAX] = {
    {PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN, PS_KING, PS_NONE, PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN, PS_KING, PS_NONE},
    {PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN, PS_KING, PS_NONE, PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN, PS_KING, PS_NONE}
};

constexpr std::uint16_t OrientTBL[SIDE_MAX][SQUARE_MAX] = {
    { H1, H1, H1, H1, A1, A1, A1, A1,
    H1, H1, H1, H1, A1, A1, A1, A1,
    H1, H1, H1, H1, A1, A1, A1, A1,
    H1, H1, H1, H1, A1, A1, A1, A1,
    H1, H1, H1, H1, A1, A1, A1, A1,
    H1, H1, H1, H1, A1, A1, A1, A1,
    H1, H1, H1, H1, A1, A1, A1, A1,
    H1, H1, H1, H1, A1, A1, A1, A1 },
    { H8, H8, H8, H8, A8, A8, A8, A8,
    H8, H8, H8, H8, A8, A8, A8, A8,
    H8, H8, H8, H8, A8, A8, A8, A8,
    H8, H8, H8, H8, A8, A8, A8, A8,
    H8, H8, H8, H8, A8, A8, A8, A8,
    H8, H8, H8, H8, A8, A8, A8, A8,
    H8, H8, H8, H8, A8, A8, A8, A8,
    H8, H8, H8, H8, A8, A8, A8, A8 }
};

#define B(v) (v * PS_NB)
constexpr std::uint16_t KingBuckets[SIDE_MAX][SQUARE_MAX] = {
    { B(28), B(29), B(30), B(31), B(31), B(30), B(29), B(28),
    B(24), B(25), B(26), B(27), B(27), B(26), B(25), B(24),
    B(20), B(21), B(22), B(23), B(23), B(22), B(21), B(20),
    B(16), B(17), B(18), B(19), B(19), B(18), B(17), B(16),
    B(12), B(13), B(14), B(15), B(15), B(14), B(13), B(12),
    B( 8), B( 9), B(10), B(11), B(11), B(10), B( 9), B( 8),
    B( 4), B( 5), B( 6), B( 7), B( 7), B( 6), B( 5), B( 4),
    B( 0), B( 1), B( 2), B( 3), B( 3), B( 2), B( 1), B( 0) },
    { B( 0), B( 1), B( 2), B( 3), B( 3), B( 2), B( 1), B( 0),
    B( 4), B( 5), B( 6), B( 7), B( 7), B( 6), B( 5), B( 4),
    B( 8), B( 9), B(10), B(11), B(11), B(10), B( 9), B( 8),
    B(12), B(13), B(14), B(15), B(15), B(14), B(13), B(12),
    B(16), B(17), B(18), B(19), B(19), B(18), B(17), B(16),
    B(20), B(21), B(22), B(23), B(23), B(22), B(21), B(20),
    B(24), B(25), B(26), B(27), B(27), B(26), B(25), B(24),
    B(28), B(29), B(30), B(31), B(31), B(30), B(29), B(28) }
};
#undef B

constexpr std::uint16_t make_index(const std::uint16_t king_square, const std::uint16_t piece_square, const std::uint16_t piece_type, int Perspective) noexcept {
    return (piece_square ^ OrientTBL[Perspective][king_square]) + PieceSquareIndex[Perspective][piece_type] + KingBuckets[Perspective][king_square];
}

}  // namespace nnue
