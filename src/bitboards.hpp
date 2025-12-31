#pragma once

#include "bitboard.hpp"
#include "side.hpp"

enum direction_e : int8_t {
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST
};

constexpr size_t DIRECTION_MAX = 8;

class bitboards {
  template <size_t MAX>
  struct slider_lookup_t;

  using dualboard = __v2du;
  using quadboard = __v4du;
  using octoboard = __v8du;
  using leaper_lookup_t = std::array<bitboard, SQUARE_MAX>;
  using line_lookup_t = std::array<leaper_lookup_t, SQUARE_MAX>;
  using ray_lookup_t = std::array<std::array<bitboard, DIRECTION_MAX>, SQUARE_MAX>;
  using slider_lookup_rook_queen_t = slider_lookup_t<102400>;
  using slider_lookup_bishop_queen_t = slider_lookup_t<5248>;

  static const line_lookup_t lookup_line;
  static const ray_lookup_t lookup_ray;
  static const leaper_lookup_t lookup_king;
  static const leaper_lookup_t lookup_knight;
  static const std::array<leaper_lookup_t, 2> lookup_pawn;
  static const slider_lookup_rook_queen_t lookup_rook_queen;
  static const slider_lookup_bishop_queen_t lookup_bishop_queen;

  static quadboard expand(quadboard in, quadboard empty) noexcept;
  static bitboard relevant_occupancy(square square, bitboard occupied) noexcept;

public:
  static constexpr bitboard ALL = ~0ull;

  static bitboard line(square from, square to) noexcept;
  static bitboard ray(square from, direction_e dir) noexcept;

  static bitboard king(square square) noexcept;
  static bitboard knight(square square) noexcept;
  static bitboard rook_queen(square square, bitboard occupied) noexcept;
  static bitboard bishop_queen(square square, bitboard occupied) noexcept;

  static bitboard leaper(bitboard king, bitboard knight) noexcept;
  static bitboard slider(bitboard rook_queen, bitboard bishop_queen, bitboard occupied) noexcept;

  template <side_e side>
  static bitboard pawn(square square) noexcept;
  static bitboard pawn(square square, side_e side) noexcept;
  template <side_e side>
  static bitboard pawn(bitboard squares) noexcept;
  static bitboard pawn(bitboard squares, side_e side) noexcept;
};

template <size_t MAX>
struct bitboards::slider_lookup_t {
  struct info_t {
    bitboard occ;
    bitboard all;
    std::uint32_t offset;
  };

  std::array<info_t, SQUARE_MAX> infos{};
  std::array<std::uint16_t, MAX> data{};

  bitboard operator()(square square, bitboard occupied) const noexcept {
    const info_t& info = infos[square];
    return _pdep_u64(data[info.offset + _pext_u64(occupied, info.occ)], info.all);
  }
};

inline bitboard bitboards::line(square from, square to) noexcept
{
  return lookup_line[from][to];
}

inline bitboard bitboards::ray(square from, direction_e direction) noexcept
{
  return lookup_ray[from][direction];
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

inline bitboard bitboards::leaper(bitboard king, bitboard knight) noexcept {
  constexpr octoboard s = {1, 8, 7, 9, 10, 17, 15, 6};
  constexpr octoboard l = {~"a"_f, ~""_f, ~"h"_f, ~"a"_f, ~"ab"_f, ~"a"_f, ~"h"_f, ~"gh"_f};
  constexpr octoboard r = {~"h"_f, ~""_f, ~"a"_f, ~"h"_f, ~"gh"_f, ~"h"_f, ~"a"_f, ~"ab"_f};
  octoboard b = {king, king, king, king, knight, knight, knight, knight};
  octoboard t = ((b << s) & l) | ((b >> s) & r);
  return __builtin_reduce_or(t);
}

inline bitboards::quadboard bitboards::expand(quadboard in, quadboard empty) noexcept {
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

const bitboards::leaper_lookup_t bitboards::lookup_king = []() noexcept {
  leaper_lookup_t lookup{};
  for (square sq : ALL)
    lookup[sq] = leaper(bitboard{sq}, bitboard{});
  return lookup;
}();

const bitboards::leaper_lookup_t bitboards::lookup_knight = []() noexcept {
  leaper_lookup_t lookup{};
  for (square sq : ALL)
    lookup[sq] = leaper(bitboard{}, bitboard{sq});
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

inline bitboard bitboards::relevant_occupancy(square square, bitboard occupied) noexcept {
  if (square.rank() > rank_e::R1)
    occupied &= ~"1"_r;
  if (square.rank() < rank_e::R8)
    occupied &= ~"8"_r;
  if (square.file() > file_e::FA)
    occupied &= ~"a"_f;
  if (square.file() < file_e::FH)
    occupied &= ~"h"_f;
  return occupied;
}

const bitboards::slider_lookup_rook_queen_t bitboards::lookup_rook_queen = []() noexcept {
  slider_lookup_rook_queen_t lookup{};
  std::uint32_t offset = 0;
  for (square sq : ALL) {
    bitboard board{sq};
    bitboard all = slider(board, 0ull, 0ull);
    bitboard occ = relevant_occupancy(sq, all);
    std::uint32_t size = 1u << occ.size();
    lookup.infos[sq] = {occ, all, offset};
    for (std::uint32_t index = 0; index < size; ++index) {
      bitboard blockers = _pdep_u64(index, occ);
      bitboard attacks  = slider(board, 0ull, blockers);
      lookup.data[offset + index] = _pext_u64(attacks, all);
    }
    offset += size;
  }
  return lookup;
}();

const bitboards::slider_lookup_bishop_queen_t bitboards::lookup_bishop_queen = []() noexcept {
  slider_lookup_bishop_queen_t lookup{};
  std::uint32_t offset = 0;
  for (square sq : ALL) {
    bitboard board{sq};
    bitboard all = slider(0ull, board, 0ull);
    bitboard occ = relevant_occupancy(sq, all);
    std::uint32_t size = 1u << occ.size();
    lookup.infos[sq] = {occ, all, offset};
    for (std::uint32_t index = 0; index < size; ++index) {
      bitboard blockers = _pdep_u64(index, occ);
      bitboard attacks  = slider(0ull, board, blockers);
      lookup.data[offset + index] = _pext_u64(attacks, all);
    }
    offset += size;
  }
  return lookup;
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

const bitboards::ray_lookup_t bitboards::lookup_ray = []() noexcept {
  ray_lookup_t lookup{};

  for (square from : ALL) {
    bitboard from_bb = bitboard{from};
    quadboard b = {from_bb, from_bb, from_bb, from_bb};
    quadboard o = {0, 0, 0, 0};
    quadboard t = expand(b, ~o);
    //  { 1, 8, 7, 9 };
    bitboard bb1 = t[0];
    bitboard bb8 = t[1];
    bitboard bb7 = t[2];
    bitboard bb9 = t[3];

    for (square to : bb1)
      lookup[from][to > from ? EAST : WEST].set(to);
    for (square to : bb8)
      lookup[from][to > from ? NORTH : SOUTH].set(to);
    for (square to : bb7)
      lookup[from][to > from ? NORTH_WEST : SOUTH_EAST].set(to);
    for (square to : bb9)
      lookup[from][to > from ? NORTH_EAST : SOUTH_WEST].set(to);
  }

  return lookup;
}();