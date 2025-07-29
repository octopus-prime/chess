#pragma once

#include "bitboard.hpp"
#include "side.hpp"

class bitboards
{
  using dualboard = __v2du;
  using quadboard = __v4du;
  using leaper_lookup_t = std::array<bitboard, 64>;
  using line_lookup_t = std::array<leaper_lookup_t, 64>;
  struct slider_lookup_t;

  static const line_lookup_t lookup_line;
  static const leaper_lookup_t lookup_king;
  static const leaper_lookup_t lookup_knight;
  // static const leaper_lookup_t lookup_pawn_white;
  // static const leaper_lookup_t lookup_pawn_black;
  static const std::array<leaper_lookup_t, 2> lookup_pawn;
  static const slider_lookup_t lookup_rook_queen;
  static const slider_lookup_t lookup_bishop_queen;

  static constexpr bitboard permutation(bitboard iteration, bitboard mask) noexcept;
  static auto expand(auto in, auto empty) noexcept;

public:
  static constexpr bitboard ALL = ~0ull;

  static bitboard line(square from, square to) noexcept;

  static bitboard king(square square) noexcept;
  static bitboard king(bitboard squares) noexcept;

  static bitboard knight(square square) noexcept;
  static bitboard knight(bitboard squares) noexcept;

  static bitboard rook_queen(square square, bitboard occupied) noexcept;
  static bitboard rook_queen(bitboard squares, bitboard occupied) noexcept;

  static bitboard bishop_queen(square square, bitboard occupied) noexcept;
  static bitboard bishop_queen(bitboard squares, bitboard occupied) noexcept;

  static bitboard slider(bitboard rook_queen, bitboard bishop_queen, bitboard occupied) noexcept;

  template <side_e side>
  static bitboard pawn(square square) noexcept;
  static bitboard pawn(square square, side_e side) noexcept;
  template <side_e side>
  static bitboard pawn(bitboard squares) noexcept;
  static bitboard pawn(bitboard squares, side_e side) noexcept;
};

struct bitboards::slider_lookup_t
{
  struct block_t
  {
    bitboard mask;
    std::vector<bitboard> data;
  };

  std::array<block_t, 64> blocks;

  bitboard
  operator()(square square, bitboard occupied) const noexcept
  {
    const auto &block = blocks[square];
    const auto index = _pext_u64(occupied, block.mask);
    return block.data[index];
  }
};

inline bitboard bitboards::line(square from, square to) noexcept
{
  return lookup_line[from][to];
}

inline bitboard bitboards::king(square square) noexcept
{
  return lookup_king[square];
}

inline bitboard bitboards::knight(square square) noexcept
{
  return lookup_knight[square];
}

inline bitboard bitboards::rook_queen(square square, bitboard occupied) noexcept
{
  return lookup_rook_queen(square, occupied);
}

inline bitboard bitboards::bishop_queen(square square, bitboard occupied) noexcept
{
  return lookup_bishop_queen(square, occupied);
}

template <>
inline bitboard bitboards::pawn<WHITE>(square square) noexcept {
  return lookup_pawn[WHITE][square];
}

template <>
inline bitboard bitboards::pawn<BLACK>(square square) noexcept {
  return lookup_pawn[BLACK][square];
}

inline bitboard bitboards::pawn(square square, side_e side) noexcept {
  return lookup_pawn[side][square];
}

inline bitboard bitboards::king(bitboard in) noexcept
{
  constexpr quadboard s = {1, 8, 7, 9};
  constexpr quadboard l = {~"a"_f, ~""_f, ~"h"_f, ~"a"_f};
  constexpr quadboard r = {~"h"_f, ~""_f, ~"a"_f, ~"h"_f};
  quadboard b = {in, in, in, in};
  quadboard t = ((b << s) & l) | ((b >> s) & r);
  return t[0] | t[1] | t[2] | t[3];
}

inline bitboard bitboards::knight(bitboard in) noexcept
{
  constexpr quadboard s = {10, 17, 15, 6};
  constexpr quadboard l = {~"ab"_f, ~"a"_f, ~"h"_f, ~"gh"_f};
  constexpr quadboard r = {~"gh"_f, ~"h"_f, ~"a"_f, ~"ab"_f};
  quadboard b = {in, in, in, in};
  quadboard t = ((b << s) & l) | ((b >> s) & r);
  return t[0] | t[1] | t[2] | t[3];
}

inline auto bitboards::expand(auto in, auto empty) noexcept {
	constexpr quadboard shift = { 1, 8, 7, 9 };
	constexpr quadboard not_left = { ~"a"_f, ~""_f, ~"h"_f, ~"a"_f };
	constexpr quadboard not_right = { ~"h"_f, ~""_f, ~"a"_f, ~"h"_f };
	quadboard left(in);
	quadboard right(in);
	quadboard board(empty & not_left);

	left |= board & (left << shift);
	board &= (board << shift);
	left |= board & (left << (shift * 2));
	board &= (board << (shift * 2));
	left |= board & (left << (shift * 4));
	left = (left << shift) & not_left;

	board = empty & not_right;

	right |= board & (right >> shift);
	board &= (board >> shift);
	right |= board & (right >> (shift * 2));
	board &= (board >> (shift * 2));
	right |= board & (right >> (shift * 4));
	right = (right >> shift) & not_right;

	return left | right;
}

inline bitboard bitboards::rook_queen(bitboard in, bitboard occupied) noexcept
{
  quadboard b = {in, in, 0, 0};
  quadboard o = {occupied, occupied, 0, 0};
  quadboard t = expand(b, ~o);
  return t[0] | t[1];
}

inline bitboard bitboards::bishop_queen(bitboard in, bitboard occupied) noexcept
{
  quadboard b = {0, 0, in, in};
  quadboard o = {0, 0, occupied, occupied};
  quadboard t = expand(b, ~o);
  return t[2] | t[3];
}

inline bitboard bitboards::slider(bitboard rook_queen, bitboard bishop_queen, bitboard occupied) noexcept
{
  quadboard b = {rook_queen, rook_queen, bishop_queen, bishop_queen};
  quadboard o = {occupied, occupied, occupied, occupied};
  quadboard t = expand(b, ~o);
  return t[0] | t[1] | t[2] | t[3];
}

template <>
inline bitboard bitboards::pawn<WHITE>(bitboard in) noexcept
{
  constexpr dualboard s = {7, 9};
  constexpr dualboard m = {~"h"_f, ~"a"_f};
  dualboard b = {in, in};
  dualboard t = (b << s) & m;
  return t[0] | t[1];
}

template <>
inline bitboard bitboards::pawn<BLACK>(bitboard in) noexcept
{
  constexpr dualboard s = {7, 9};
  constexpr dualboard m = {~"a"_f, ~"h"_f};
  dualboard b = {in, in};
  dualboard t = (b >> s) & m;
  return t[0] | t[1];
}

inline bitboard bitboards::pawn(bitboard in, side_e side) noexcept {
  return side == WHITE ? pawn<WHITE>(in) : pawn<BLACK>(in);
}

constexpr bitboard bitboards::permutation(bitboard iteration, bitboard mask) noexcept {
  bitboard blockers = 0ull;
  while (iteration != 0ull) {
    if ((iteration & bitboard{1ull}) != bitboard{0ull}) {
      blockers |= bitboard{mask.front()};
    }
    iteration >>= 1;
    mask &= (mask - 1ull);
  }
  return blockers;
}

const bitboards::leaper_lookup_t bitboards::lookup_king = []() noexcept {
  leaper_lookup_t lookup{};
  for (square sq : ALL)
    lookup[sq] = king(bitboard{sq});
  return lookup;
}();

const bitboards::leaper_lookup_t bitboards::lookup_knight = []() noexcept {
  leaper_lookup_t lookup{};
  for (square sq : ALL)
    lookup[sq] = knight(bitboard{sq});
  return lookup;
}();

const std::array<bitboards::leaper_lookup_t, 2> bitboards::lookup_pawn = []() noexcept {
  std::array<bitboards::leaper_lookup_t, 2> lookup{};
  for (square sq : ALL) {
    lookup[WHITE][sq] = pawn<WHITE>(bitboard{sq});
    lookup[BLACK][sq] = pawn<BLACK>(bitboard{sq});
  }
  return lookup;
}();

// const bitboards::leaper_lookup_t bitboards::lookup_pawn_white = []() noexcept {
//   leaper_lookup_t lookup{};
//   for (square sq : ALL)
//     lookup[sq] = pawn<WHITE>(bitboard{sq});
//   return lookup;
// }();

// const bitboards::leaper_lookup_t bitboards::lookup_pawn_black = []() noexcept {
//   leaper_lookup_t lookup{};
//   for (square sq : ALL)
//     lookup[sq] = pawn<BLACK>(bitboard{sq});
//   return lookup;
// }();

const bitboards::slider_lookup_t bitboards::lookup_rook_queen = []() noexcept {
  std::array<slider_lookup_t::block_t, 64> blocks {};
  for (square sq : ALL) {
    bitboard board{sq};
    bitboard rooks = rook_queen(board, 0ull);
    auto size = 1ull << rooks.size();
    blocks[sq].data.resize(size);
    for (std::uint64_t index = 0; index < size; ++index) {
      bitboard blockers = permutation(index, rooks);
      blocks[sq].data[index] = rook_queen(board, blockers);
    }
    blocks[sq].mask = rooks;
  }
  return slider_lookup_t{blocks};
}();

const bitboards::slider_lookup_t bitboards::lookup_bishop_queen = []() noexcept {
  std::array<slider_lookup_t::block_t, 64> blocks {};
  for (square sq : ALL) {
    bitboard board{sq};
    bitboard bishops = bishop_queen(board, 0ull);
    auto size = 1ull << bishops.size();
    blocks[sq].data.resize(size);
    for (std::uint64_t index = 0; index < size; ++index) {
      bitboard blockers = permutation(index, bishops);
      blocks[sq].data[index] = bishop_queen(board, blockers);
    }
    blocks[sq].mask = bishops;
  }
  return slider_lookup_t{blocks};
}();

const bitboards::line_lookup_t bitboards::lookup_line = []() noexcept {
  line_lookup_t lookup{};
  for (square from : ALL) {
    for (square to : ALL) {
      lookup[from][to] = bitboard{from} | bitboard{to};
      if (from == to)
        continue;
      bitboard board = bishop_queen(from, 0ull) | rook_queen(from, 0ull);
      if (!board[to])
        continue;
      for (square sq = from; sq != to; ) {
        lookup[from][to] |= bitboard{static_cast<square>(static_cast<square_e>(sq & 63))};
        sq += (from.file() == to.file()) ? 0 : (from.file() > to.file()) ? -1 : +1;
        sq += (from.rank() == to.rank()) ? 0 : (from.rank() > to.rank()) ? -8 : +8;
      }
    }
  }
  return lookup;
}();
