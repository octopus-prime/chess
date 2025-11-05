#pragma once

#include "bitboard.hpp"
#include "bitboards.hpp"
#include "piece.hpp"
#include "hashes.hpp"
#include "move.hpp"
#include <vector>
#include <print>
#include <charconv>

// use once per thread
struct position_t {

    enum {
        MAX_MOVES_PER_PLY = 218,
        MAX_ACTIVE_MOVES_PER_PLY = 48,
        MAX_MOVES_PER_GAME = 1024
    };

    // constexpr static size_t MAX_MOVES_PER_PLY = 218;
    // constexpr static size_t MAX_MOVES_PER_GAME = 1024;

    // use once per ply
    struct alignas (64) state_t {
        bitboard castle;
        // bitboard en_passant;
        bitboard checkers; // attackers unblocked for current side
        bitboard snipers[SIDE_MAX];  // attackers blocked by exactly one piece
        bitboard blockers[SIDE_MAX]; // blockers of snipers
        hash_t hash;
        square en_passant;
        uint8_t half_move;
        uint8_t repetition;
        piece captured;    
        // bool null_move;
        move_t last_move;

        // bool operator==(const state_t& other) const noexcept = default;
    };

    static_assert(sizeof(state_t) == 64);

    constexpr static std::string_view STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv;

    position_t() noexcept;
    position_t(std::string_view fen) noexcept;

    bool operator==(const position_t& other) const noexcept = default;

    void make_move(move_t move) noexcept;
    void undo_move(move_t move) noexcept;

    bool can_null_move() const noexcept;
    void make_null_move() noexcept;
    void undo_null_move() noexcept;

    bitboard attackers(side_e side) const noexcept;
    bitboard attackers(square square) const noexcept;

    std::span<move_t> generate_all_moves(std::span<move_t, MAX_MOVES_PER_PLY> buffer) const noexcept {
        // return generate_moves(buffer, bitboards::ALL, 0ull, {QUEEN, ROOK, BISHOP, KNIGHT});
        constexpr static std::array<bitboard, TYPE_MAX> empty_check_targets = []{
            std::array<bitboard, TYPE_MAX> arr{};
            arr.fill(0ull);
            return arr;
        }();
        return generate_moves(buffer, bitboards::ALL, 0ull, {QUEEN, ROOK, BISHOP, KNIGHT}, empty_check_targets);
    }

    std::span<move_t> generate_active_moves(std::span<move_t, MAX_ACTIVE_MOVES_PER_PLY> buffer) const noexcept {
        bitboard promotion_targets = side == WHITE ? by(side, PAWN) << 8 & ~by() & "8"_r : by(side, PAWN) >> 8 & ~by() & "1"_r;

        square ksq = by(~side, KING).front();
        bitboard b = bitboards::bishop_queen(ksq, by()) & ~by(side);
        bitboard r = bitboards::rook_queen(ksq, by()) & ~by(side);
        std::array<bitboard, TYPE_MAX> check_targets = {
            0ull, // NO_TYPE
            bitboards::pawn(ksq, ~side) & ~by(side), // PAWN
            bitboards::knight(ksq) & ~by(side), // KNIGHT
            b,
            r,
            r | b,
            0ull, // KING
            0ull
        };

        // std::println("b: {}", b);
        // std::println("r: {}", r);
        // std::println("B: {}", check_targets[BISHOP]);
        // std::println("R: {}", check_targets[ROOK]);
        // std::println("Q: {}", check_targets[QUEEN]);

        return generate_moves(buffer, by(~side), promotion_targets, {QUEEN, KNIGHT}, check_targets);
    }

    int16_t see(move_t move) const noexcept;

    position_t& operator=(std::string_view fen) noexcept {
        board.fill(NO_PIECE);
        occupied_by_type.fill(0);
        occupied_by_side.fill(0);
        material.fill(0);
        full_move = 0;
        side = WHITE;
        states.clear();
        setup(fen);
        return *this;
    }

    piece operator[](square s) const noexcept {
        return board[s];
    }

    piece at(square s) const noexcept {
        return board[s];
    }

    bitboard by() const noexcept {
        return occupied_by_side[WHITE] | occupied_by_side[BLACK];
    }

    bitboard by(piece p) const noexcept {
        return occupied_by_type[p.type()] & occupied_by_side[p.side()];
    }

    bitboard by(side_e side, type_e type) const noexcept {
        return occupied_by_type[type] & occupied_by_side[side];
    }

    bitboard by(side_e side) const noexcept {
        return occupied_by_side[side];
    }

    bitboard by(type_e type) const noexcept {
        return occupied_by_type[type];
    }

    bitboard by(type_e type, type_e type2) const noexcept {
        return occupied_by_type[type] | occupied_by_type[type2];
    }

    bitboard by(side_e side, type_e type, type_e type2) const noexcept {
        return occupied_by_side[side] & (occupied_by_type[type] | occupied_by_type[type2]);
    }

    int get_material() const noexcept {
        return material[side] - material[~side];
    }

    side_e get_side() const noexcept {
        return side;
    }

    uint16_t get_half_move() const noexcept {
        return states.back().half_move;
    }

    // uint16_t get_full_move() const noexcept {
    //     return states.back().full_move;
    // }

    // bitboard castle() const noexcept {
    //     return states.back().castle;
    // }

    // bitboard en_passant() const noexcept {
    //     return states.back().en_passant;
    // }

    hash_t hash() const noexcept {
        return states.back().hash;
    }

    move_t last_move() const noexcept {
        return states.back().last_move;
    }

    bool is_check() const noexcept {
        return !states.back().checkers.empty();
    }

    bool is_50_moves_rule() const noexcept {
        return states.back().half_move >= 100;
    }

    bool is_3_fold_repetition() const noexcept {
        return states.back().repetition >= 3;
    }

    bool is_no_material() const noexcept {
        return by(PAWN).empty() && material[WHITE] <= 10300 && material[BLACK] <= 10300;
    }

    bool check(move_t move) const noexcept {
        square king_square = by(~side, KING).front();
        square to = move.to();
        type_e type = move.promotion() == NO_TYPE ? at(move.from()).type() : move.promotion();
        switch (type) {
            case PAWN:
            return bitboards::pawn(king_square, ~side) & bitboard{to};
            case KNIGHT:
            return bitboards::knight(king_square) & bitboard{to};
            case BISHOP:
            return bitboards::bishop_queen(king_square, by()) & bitboard{to};
            case ROOK:
            return bitboards::rook_queen(king_square, by()) & bitboard{to};
            case QUEEN:
            return (bitboards::bishop_queen(king_square, by()) | bitboards::rook_queen(king_square, by())) & bitboard{to};
            default:
            return false;
        }
    }

private:
    std::span<move_t> generate_moves(std::span<move_t> buffer, bitboard valid_targets, bitboard promotion_targets, std::initializer_list<type_e> promotion_types, std::span<const bitboard, TYPE_MAX> check_targets) const noexcept;
    void setup(std::string_view fen) noexcept;
    std::tuple<bitboard, bitboard> find_snipers_and_blockers(side_e side) const noexcept;
    uint8_t find_repetitions(hash_t hash, uint8_t half_move) const noexcept;

    std::array<piece, SQUARE_MAX> board;
    std::array<bitboard, 7> occupied_by_type;
    std::array<bitboard, SIDE_MAX> occupied_by_side;
    std::array<int16_t, SIDE_MAX> material;
    uint16_t full_move;
    side_e side;
    std::vector<state_t> states;
};

inline position_t::position_t() noexcept : position_t{STARTPOS} {
}

inline position_t::position_t(std::string_view fen) noexcept : board{}, occupied_by_type{}, occupied_by_side{}, material{}, full_move{}, side{WHITE}, states{} {
    states.reserve(MAX_MOVES_PER_GAME);
    setup(fen);
}

std::tuple<bitboard, bitboard> position_t::find_snipers_and_blockers(side_e side) const noexcept {
    bitboard snipers;
    bitboard blockers;

    square ksq = by(side, KING).front();

    auto scan = [&](direction_e direction, bitboard candidates, auto&& find_nearest) {
        bitboard ray = bitboards::ray(ksq, direction);
        bitboard sniper_candidates = ray & candidates;
        if (!sniper_candidates.empty()) {
            square sniper = std::invoke(find_nearest, sniper_candidates);
            bitboard line = bitboards::line(ksq, sniper);
            bitboard between = line & by() & ~bitboard{ksq} & ~bitboard{sniper};
            bitboard blocker_candidates = between & by(side);
            if (between.size() == 1 && blocker_candidates.size() == 1) {
                square blocker = blocker_candidates.front();
                snipers.set(sniper);
                blockers.set(blocker);
            }
        }
    };

    scan(NORTH, by(~side, ROOK, QUEEN), &bitboard::front);
    scan(SOUTH, by(~side, ROOK, QUEEN), &bitboard::back);
    scan(EAST, by(~side, ROOK, QUEEN), &bitboard::front);
    scan(WEST, by(~side, ROOK, QUEEN), &bitboard::back);
    scan(NORTH_EAST, by(~side, BISHOP, QUEEN), &bitboard::front);
    scan(NORTH_WEST, by(~side, BISHOP, QUEEN), &bitboard::front);
    scan(SOUTH_EAST, by(~side, BISHOP, QUEEN), &bitboard::back);
    scan(SOUTH_WEST, by(~side, BISHOP, QUEEN), &bitboard::back);

    return {snipers, blockers};
}

inline void position_t::setup(std::string_view fen) noexcept {
    constexpr auto castle_lookup = [](char ch) -> bitboard {
        switch (ch) {
            case 'K': return "h1"_b; case 'k': return "h8"_b;
            case 'Q': return "a1"_b; case 'q': return "a8"_b;
            default: return ""_b;
        }
    };

    state_t new_state{};

    auto fen_parts = fen | std::views::split(' ');
    auto fen_part = fen_parts.begin();

    std::string_view board_part {*fen_part++};
    for (auto [rank_index, rank_part] : std::views::enumerate(board_part | std::views::split('/'))) {
        int file_index = 0;
        for (auto ch : rank_part) {
            if (std::isdigit(ch)) {
                file_index += ch - '0';
            } else {
                square square{static_cast<file_e>(file_index++), static_cast<rank_e>(7 - rank_index)};
                piece piece{std::string_view{&ch, 1}};
                board[square] = piece;
                occupied_by_type[piece.type()] |= bitboard{square};
                occupied_by_side[piece.side()] |= bitboard{square};
                material[piece.side()] += type_values[piece.type()];
                new_state.hash ^= hashes::hash(piece, square);
            }
        }
    }

    std::string_view side_part {*fen_part++};
    if (side_part[0] == 'w') {
        side = WHITE;
    } else {
        side = BLACK;
        new_state.hash ^= hashes::side();
    }

    std::string_view castle_part {*fen_part++};
    if (castle_part[0] != '-') {
        for (auto ch : castle_part) {
            new_state.castle |= castle_lookup(ch);
        }
        new_state.hash ^= hashes::castle(new_state.castle);
    }

    std::string_view en_passant_part {*fen_part++};
    if (en_passant_part[0] != '-') {
        new_state.en_passant = en_passant_part;
        new_state.hash ^= hashes::en_passant(new_state.en_passant);
    } else {
        new_state.en_passant = NO_SQUARE;
    }

    if (fen_part != fen_parts.end()) {
        std::string_view half_move_part {*fen_part++};
        std::from_chars(&*half_move_part.begin(), &*half_move_part.end(), new_state.half_move);
    }

    if (fen_part != fen_parts.end()) {
        std::string_view full_move_part {*fen_part++};
        std::from_chars(&*full_move_part.begin(), &*full_move_part.end(), full_move);
    }

    auto king_square = by(side, KING).front();
    new_state.checkers = attackers(king_square) & by(~side);

    for (side_e side : {WHITE, BLACK}) {
        auto [snipers, blockers] = find_snipers_and_blockers(side);
        new_state.snipers[side] = snipers;
        new_state.blockers[side] = blockers;
    }

    new_state.repetition = 1;

    new_state.captured = NO_PIECE;
    new_state.last_move = move_t{};

    states.push_back(new_state);
}

inline bitboard position_t::attackers(side_e side) const noexcept {
    bitboard attackers = 0ull;
    attackers |= bitboards::king(by(side, KING));
    attackers |= bitboards::knight(by(side, KNIGHT));
    attackers |= bitboards::slider(by(side, ROOK, QUEEN), by(side, BISHOP, QUEEN), by() & ~by(~side, KING));
    attackers |= bitboards::pawn(by(side, PAWN), side);
    return attackers;
}

inline bitboard position_t::attackers(square square) const noexcept {
    bitboard attackers = 0ull;
    attackers |= bitboards::king(square) & by(KING);
    attackers |= bitboards::knight(square) & by(KNIGHT);
    attackers |= bitboards::rook_queen(square, by()) & by(ROOK, QUEEN);
    attackers |= bitboards::bishop_queen(square, by()) & by(BISHOP, QUEEN);
    attackers |= bitboards::pawn(square, WHITE) & by(BPAWN);
    attackers |= bitboards::pawn(square, BLACK) & by(WPAWN);
    return attackers;
}

inline size_t write_moves(std::span<move_t> buffer, uint32_t mask, __m512i moves) {
   _mm512_mask_compressstoreu_epi16(buffer.data(), mask, moves);
    return std::popcount(mask);
}

inline size_t splat_moves(std::span<move_t> buffer, square from, bitboard targets) {
    constexpr square_e placeholder{0};
    alignas(64) static constexpr auto table = [] {
        std::array<move_t, 64> table{};
        for (square to : bitboards::ALL)
            table[to] = move_t{placeholder, to};
        return table;
    }();

    const auto sources = _mm512_set1_epi16(move_t{from, placeholder});
    const auto moves = span_cast<const __m512i>(std::span{table});

    size_t index = 0;
    index += write_moves(buffer.subspan(index), targets >> 0,  _mm512_or_si512(moves[0], sources));
    index += write_moves(buffer.subspan(index), targets >> 32, _mm512_or_si512(moves[1], sources));
    return index;
}

template<int offset>
inline size_t splat_pawn_moves(std::span<move_t> buffer, bitboard targets) noexcept {
    alignas(64) static constexpr auto table = [] {
        std::array<move_t, 64> table{};
        for (square to : bitboards::ALL) {
            square_e from {(int8_t) std::clamp<int>(to + offset, 0, 63)};
            table[to] = move_t{from, to};
        }
        return table;
    }();

    const auto moves = span_cast<const __m512i>(std::span{table});

    size_t index = 0;
    index += write_moves(buffer.subspan(index), targets >> 0,  moves[0]);
    index += write_moves(buffer.subspan(index), targets >> 32, moves[1]);
    return index;
}

inline std::span<move_t> position_t::generate_moves(std::span<move_t> buffer, bitboard valid_targets, bitboard promotion_targets, std::initializer_list<type_e> promotion_types, std::span<const bitboard, TYPE_MAX> check_targets) const noexcept {

    size_t index = 0;
    bitboard checkers = states.back().checkers;
    bitboard en_passant = states.back().en_passant == NO_SQUARE ? bitboard{} : bitboard{states.back().en_passant};
    bitboard attacked = attackers(~side);
    bitboard valid_en_passant = 0ull;
    square ksq = by(side, KING).front();

    // std::println("attacked:\n{:b}", attacked);

    // for (square to_square : bitboards::king(ksq) & ~attacked & ~by(side) & valid_targets) {
    //     buffer[index++] = {ksq, to_square};
    // }
    index += splat_moves(buffer.subspan(index), ksq, bitboards::king(ksq) & ~attacked & ~by(side) & valid_targets);

    if (checkers.size() > 1) {
        return buffer.first(index);
    }

    if (checkers.size() == 1) {
        square from = by(side, KING).front();
        square to = checkers.front();
        valid_targets = bitboards::line(from, to);
        if (side == BLACK) {
            if (checkers == en_passant << 8)
                valid_en_passant = en_passant;
        } else {
            if (checkers == en_passant >> 8)
                valid_en_passant = en_passant;
        }
    }

    if (valid_targets == bitboards::ALL) {
        bitboard castle = states.back().castle;
        if (side == WHITE) {
            if ((castle & "h1"_b) && !(by() & "f1g1"_b) && !(attacked & "e1f1g1"_b))
                buffer[index++] = {E1, G1};
            if ((castle & "a1"_b) && !(by() & "b1c1d1"_b) && !(attacked & "e1d1c1"_b))
                buffer[index++] = {E1, C1};
        } else {
            if ((castle & "h8"_b) && !(by() & "f8g8"_b) && !(attacked & "e8f8g8"_b))
                buffer[index++] = {E8, G8};
            if ((castle & "a8"_b) && !(by() & "b8c8d8"_b) && !(attacked & "e8d8c8"_b))
                buffer[index++] = {E8, C8};
        }
    }

    bitboard snipers = states.back().snipers[side];
    bitboard pinned = states.back().blockers[side];
    bitboard valid_for_pinned[64];
    std::ranges::fill(valid_for_pinned, ~0ull);

    for (square sq : snipers) {
        bitboard line = bitboards::line(ksq, sq);
        square blocker = (line & pinned).front();
        valid_for_pinned[blocker] = line;
    }

    valid_targets &= ~(by(side) | by(KING));

    for (square from_square : by(side, KNIGHT) & ~pinned) {
        index += splat_moves(buffer.subspan(index), from_square, bitboards::knight(from_square) & (valid_targets | check_targets[KNIGHT]));
    }

    for (square from_square : by(side, ROOK, QUEEN)) {
         index += splat_moves(buffer.subspan(index), from_square, bitboards::rook_queen(from_square, by()) & (valid_targets | check_targets[at(from_square).type()]) & valid_for_pinned[from_square]);
    }

    for (square from_square : by(side, BISHOP, QUEEN)) {
        index += splat_moves(buffer.subspan(index), from_square, bitboards::bishop_queen(from_square, by()) & (valid_targets | check_targets[at(from_square).type()]) & valid_for_pinned[from_square]);
    }

    // const auto generate_normal = [&](bitboard targets, int delta) {
    //     for (square to : targets)
    //         if (valid_for_pinned[to + delta] & bitboard{to})
    //             buffer[index++] = {to + delta, to};
    // };

    // const auto generate_promotion = [&](bitboard targets, int delta) {
    //     for (square to : targets)
    //         if (valid_for_pinned[to + delta] & bitboard{to})
    //             for (type_e type : promotion_types)
    //                 buffer[index++] = {to + delta, to, type};
    // };

    bitboard pawns = by(side, PAWN);
//     bitboard pawns1 = pawns & ~pinned;
//     bitboard pawns2 = pawns & pinned;

//     if (side == WHITE) {
//         bitboard push = pawns1 << 8 & ~by();
//         bitboard targets;

//         targets = push & (valid_targets | promotion_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<-8>(buffer.subspan(index), targets & ~"8"_r);
//         // generate_normal(targets & ~"8"_r, -8);
//         generate_promotion(targets & "8"_r, -8);

//         targets = push << 8 & ~by() & "4"_r & (valid_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<-16>(buffer.subspan(index), targets);
//         // generate_normal(targets, -16);

//         targets = pawns1 << 7 & ~"h"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<-7>(buffer.subspan(index), targets & ~"8"_r);
//         // generate_normal(targets & ~"8"_r, -7);
//         generate_promotion(targets & "8"_r, -7);

//         targets = pawns1 << 9 & ~"a"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<-9>(buffer.subspan(index), targets & ~"8"_r);
//         // generate_normal(targets & ~"8"_r, -9);
//         generate_promotion(targets & "8"_r, -9);

//         // --
//         push = pawns2 << 8 & ~by();

//         targets = push & (valid_targets | promotion_targets | check_targets[PAWN]);
//         generate_normal(targets & ~"8"_r, -8);
//         generate_promotion(targets & "8"_r, -8);

//         targets = push << 8 & ~by() & "4"_r & (valid_targets | check_targets[PAWN]);
//         generate_normal(targets, -16);

//         targets = pawns2 << 7 & ~"h"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         generate_normal(targets & ~"8"_r, -7);
//         generate_promotion(targets & "8"_r, -7);

//         targets = pawns2 << 9 & ~"a"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         generate_normal(targets & ~"8"_r, -9);
//         generate_promotion(targets & "8"_r, -9);
//     } else {
//         bitboard push = pawns1 >> 8 & ~by();
//         bitboard targets;

//         targets = push & (valid_targets | promotion_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<+8>(buffer.subspan(index), targets & ~"1"_r);
//         // generate_normal(targets & ~"1"_r, +8);
//         generate_promotion(targets & "1"_r, +8);

//         targets = push >> 8 & ~by() & "5"_r & (valid_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<+16>(buffer.subspan(index), targets);
//         // generate_normal(targets, +16);

//         targets = pawns1 >> 7 & ~"a"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<+7>(buffer.subspan(index), targets & ~"1"_r);
//         // generate_normal(targets & ~"1"_r, +7);
//         generate_promotion(targets & "1"_r, +7);

//         targets = pawns1 >> 9 & ~"h"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         index += splat_pawn_moves<+9>(buffer.subspan(index), targets & ~"1"_r);
//         // generate_normal(targets & ~"1"_r, +9);
//         generate_promotion(targets & "1"_r, +9);

// // --
//         push = pawns2 >> 8 & ~by();

//         targets = push & (valid_targets | promotion_targets | check_targets[PAWN]);
//         generate_normal(targets & ~"1"_r, +8);
//         generate_promotion(targets & "1"_r, +8);

//         targets = push >> 8 & ~by() & "5"_r & (valid_targets | check_targets[PAWN]);
//         generate_normal(targets, +16);

//         targets = pawns2 >> 7 & ~"a"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         generate_normal(targets & ~"1"_r, +7);
//         generate_promotion(targets & "1"_r, +7);

//         targets = pawns2 >> 9 & ~"h"_f & by(~side) & (valid_targets | check_targets[PAWN]);
//         generate_normal(targets & ~"1"_r, +9);
//         generate_promotion(targets & "1"_r, +9);
//     }






    // const bitboard pawns  = by(side, PAWN);
    // const bitboard pinned = states.back().blockers[side];
    const bitboard free_pawns   = pawns & ~pinned;
    const bitboard pinned_pawns = pawns &  pinned;

    const bitboard empty  = ~by();
    const bitboard enemy  = by(~side);
    const bitboard promoR = (side == WHITE) ? "8"_r : "1"_r;
    const bitboard stepR  = (side == WHITE) ? "3"_r : "6"_r; // single-push landing rank for double push filter

    // Helper for pinned pawns (few → cheap)
    auto gen_pinned = [&](int delta, bitboard to_mask) {
        for (square f : pinned_pawns) {
            square t = static_cast<square_e>(static_cast<int>(f) + delta);
            if (!(to_mask & bitboard{t})) continue;
            if (!(valid_for_pinned[f] & bitboard{t})) continue;
            if (promoR & bitboard{t}) {
                for (type_e pt : promotion_types) buffer[index++] = {f, t, pt};
            } else {
                buffer[index++] = {f, t};
            }
        }
    };

    // Free pawns: splat non-promotions, scalar promotions
    if (side == WHITE) {
        bitboard one = (free_pawns << 8) & empty;
        bitboard to  = one & (valid_targets | promotion_targets | check_targets[PAWN]);
        index += splat_pawn_moves<-8>(buffer.subspan(index), to & ~promoR);
        for (square t : to & promoR) {
            square f = t - 8;
            if (valid_for_pinned[f] & bitboard{t})
                for (type_e pt : promotion_types) buffer[index++] = {f, t, pt};
        }
        bitboard two = ((one & stepR) << 8) & empty & (valid_targets | check_targets[PAWN]);
        index += splat_pawn_moves<-16>(buffer.subspan(index), two);

        bitboard lc = ((free_pawns << 7) & ~"h"_f & enemy) & (valid_targets | check_targets[PAWN]);
        index += splat_pawn_moves<-7>(buffer.subspan(index), lc & ~promoR);
        for (square t : lc & promoR) {
            square f = t - 7;
            if (valid_for_pinned[f] & bitboard{t})
                for (type_e pt : promotion_types) buffer[index++] = {f, t, pt};
        }

        bitboard rc = ((free_pawns << 9) & ~"a"_f & enemy) & (valid_targets | check_targets[PAWN]);
        index += splat_pawn_moves<-9>(buffer.subspan(index), rc & ~promoR);
        for (square t : rc & promoR) {
            square f = t - 9;
            if (valid_for_pinned[f] & bitboard{t})
                for (type_e pt : promotion_types) buffer[index++] = {f, t, pt};
        }

        // Pinned pawns (scalar, pin-safe)
        bitboard p_one = ((pinned_pawns << 8) & empty) & (valid_targets | promotion_targets | check_targets[PAWN]);
        gen_pinned(+8, p_one);
        bitboard p_two = (((p_one & stepR) << 8) & empty) & (valid_targets | check_targets[PAWN]);
        gen_pinned(+16, p_two);
        bitboard p_lc  = ((pinned_pawns << 7) & ~"h"_f & enemy) & (valid_targets | check_targets[PAWN]);
        gen_pinned(+7, p_lc);
        bitboard p_rc  = ((pinned_pawns << 9) & ~"a"_f & enemy) & (valid_targets | check_targets[PAWN]);
        gen_pinned(+9, p_rc);
    } else { // BLACK
        bitboard one = (free_pawns >> 8) & empty;
        bitboard to  = one & (valid_targets | promotion_targets | check_targets[PAWN]);
        index += splat_pawn_moves<+8>(buffer.subspan(index), to & ~promoR);
        for (square t : to & promoR) {
            square f = t + 8;
            if (valid_for_pinned[f] & bitboard{t})
                for (type_e pt : promotion_types) buffer[index++] = {f, t, pt};
        }
        bitboard two = ((one & stepR) >> 8) & empty & (valid_targets | check_targets[PAWN]);
        index += splat_pawn_moves<+16>(buffer.subspan(index), two);

        bitboard lc = ((free_pawns >> 7) & ~"a"_f & enemy) & (valid_targets | check_targets[PAWN]);
        index += splat_pawn_moves<+7>(buffer.subspan(index), lc & ~promoR);
        for (square t : lc & promoR) {
            square f = t + 7;
            if (valid_for_pinned[f] & bitboard{t})
                for (type_e pt : promotion_types) buffer[index++] = {f, t, pt};
        }

        bitboard rc = ((free_pawns >> 9) & ~"h"_f & enemy) & (valid_targets | check_targets[PAWN]);
        index += splat_pawn_moves<+9>(buffer.subspan(index), rc & ~promoR);
        for (square t : rc & promoR) {
            square f = t + 9;
            if (valid_for_pinned[f] & bitboard{t})
                for (type_e pt : promotion_types) buffer[index++] = {f, t, pt};
        }

        // Pinned pawns (scalar, pin-safe)
        bitboard p_one = ((pinned_pawns >> 8) & empty) & (valid_targets | promotion_targets | check_targets[PAWN]);
        gen_pinned(-8, p_one);
        bitboard p_two = (((p_one & stepR) >> 8) & empty) & (valid_targets | check_targets[PAWN]);
        gen_pinned(-16, p_two);
        bitboard p_lc  = ((pinned_pawns >> 7) & ~"a"_f & enemy) & (valid_targets | check_targets[PAWN]);
        gen_pinned(-7, p_lc);
        bitboard p_rc  = ((pinned_pawns >> 9) & ~"h"_f & enemy) & (valid_targets | check_targets[PAWN]);
        gen_pinned(-9, p_rc);
    }




    for (square from : pawns & bitboards::pawn(en_passant & (valid_targets | valid_en_passant), ~side)) {
        if (valid_for_pinned[from] & en_passant) {
            square esq = en_passant.front();
            square psq = side == WHITE ? esq - 8 : esq + 8;
            if (ksq.rank() == psq.rank()) {
                bitboard occ = by() & ~(bitboard{psq} | bitboard{from});
                if (bitboards::rook_queen(ksq, occ) & by(~side, ROOK, QUEEN))
                    continue;
            }
            buffer[index++] = {from, esq};
        }
    }

    return buffer.first(index);
}

inline void position_t::make_move(move_t move) noexcept {
    // Snapshot previous state, we’ll push the updated one at the end
    const state_t& prev = states.back();
    state_t st = prev;

    const square from = move.from();
    const square to   = move.to();

    piece moving = board[from];
    piece captured = board[to];

    const type_e prom = move.promotion();
    const bool is_promotion = (prom != NO_TYPE);
    const bool moving_is_pawn = (moving.type() == PAWN);
    const bool moving_is_king = (moving.type() == KING);
    const bool moving_is_rook = (moving.type() == ROOK);

    const bitboard from_bb{from};
    const bitboard to_bb{to};

    // Clear side-to-move’s EP from hash/state
    st.hash ^= hashes::en_passant(st.en_passant);
    st.en_passant = NO_SQUARE;

    // EP capture (special, do once, no swap)
    if (moving_is_pawn && prev.en_passant == to) {
        const square victim_sq = (side == WHITE) ? to - 8 : to + 8;
        piece victim = board[victim_sq]; // must be opponent pawn

        // Remove victim from board/bitboards/material/hash
        board[victim_sq] = NO_PIECE;
        occupied_by_type[PAWN].reset(victim_sq);
        occupied_by_side[~side].reset(victim_sq);
        st.hash ^= hashes::hash(victim, victim_sq);
        material[~side] -= type_values[PAWN];

        captured = victim; // for state bookkeeping
        st.half_move = 0;
    } else if (captured != NO_PIECE) {
        // Normal capture (at 'to')
        occupied_by_type[captured.type()].reset(to);
        occupied_by_side[captured.side()].reset(to);
        st.hash ^= hashes::hash(captured, to);
        material[~side] -= type_values[captured.type()];
        st.half_move = 0;

        // If captured a rook on its original square, update castle rights
        if (captured.type() == ROOK) {
            if (to == H1 || to == A1 || to == H8 || to == A8) {
                const bitboard old = st.castle;
                st.castle.reset(bitboard{to} & "a1h1a8h8"_b);
                if (old != st.castle) {
                    st.hash ^= hashes::castle(old);
                    st.hash ^= hashes::castle(st.castle);
                }
            }
        }
    }

    // Move piece on board
    board[from] = NO_PIECE;

    piece final_piece = moving;
    if (is_promotion) {
        final_piece = piece{side, prom};
    }
    board[to] = final_piece;

    // Update occupancy for mover (handle promotion vs normal)
    if (is_promotion) {
        // Remove pawn at from
        occupied_by_type[PAWN].reset(from);
        occupied_by_side[side].reset(from);
        st.hash ^= hashes::hash(moving, from);

        // Add promoted piece at to
        occupied_by_type[final_piece.type()].set(to);
        occupied_by_side[side].set(to);
        st.hash ^= hashes::hash(final_piece, to);

        // Material delta (replace pawn by prom)
        material[side] += (type_values[final_piece.type()] - type_values[PAWN]);
        st.half_move = 0; // promotion resets 50-move
    } else {
        // Normal move for the moving piece
        occupied_by_type[moving.type()].reset(from);
        occupied_by_type[moving.type()].set(to);
        occupied_by_side[side].reset(from);
        occupied_by_side[side].set(to);
        st.hash ^= hashes::hash(moving, from);
        st.hash ^= hashes::hash(final_piece, to);
    }

    // Castling (king moves rook)
    if (moving_is_king) {
        if (from == E1 && to == G1) {
            // White O-O
            board[F1] = WROOK; board[H1] = NO_PIECE;
            occupied_by_type[ROOK].flip("f1h1"_b);
            occupied_by_side[WHITE].flip("f1h1"_b);
            st.hash ^= hashes::hash(WROOK, F1) ^ hashes::hash(WROOK, H1);
        } else if (from == E1 && to == C1) {
            // White O-O-O
            board[D1] = WROOK; board[A1] = NO_PIECE;
            occupied_by_type[ROOK].flip("a1d1"_b);
            occupied_by_side[WHITE].flip("d1a1"_b);
            st.hash ^= hashes::hash(WROOK, A1) ^ hashes::hash(WROOK, D1);
        } else if (from == E8 && to == G8) {
            // Black O-O
            board[F8] = BROOK; board[H8] = NO_PIECE;
            occupied_by_type[ROOK].flip("f8h8"_b);
            occupied_by_side[BLACK].flip("f8h8"_b);
            st.hash ^= hashes::hash(BROOK, F8) ^ hashes::hash(BROOK, H8);
        } else if (from == E8 && to == C8) {
            // Black O-O-O
            board[D8] = BROOK; board[A8] = NO_PIECE;
            occupied_by_type[ROOK].flip("a8d8"_b);
            occupied_by_side[BLACK].flip("d8a8"_b);
            st.hash ^= hashes::hash(BROOK, A8) ^ hashes::hash(BROOK, D8);
        }

        // Update castle rights on king move
        const bitboard old = st.castle;
        if (side == WHITE)
            st.castle.reset("a1h1"_b);
        else
            st.castle.reset("a8h8"_b);
        if (old != st.castle) {
            st.hash ^= hashes::castle(old);
            st.hash ^= hashes::castle(st.castle);
        }
    } else if (moving_is_rook) {
        // Rook moves off its original square -> update rights
        if (from == H1 || from == A1 || from == H8 || from == A8) {
            const bitboard old = st.castle;
            st.castle.reset(bitboard{from} & "a1h1a8h8"_b);
            if (old != st.castle) {
                st.hash ^= hashes::castle(old);
                st.hash ^= hashes::castle(st.castle);
            }
        }
    }

    // New EP square after double pawn push
    if (moving_is_pawn) {
        const int delta = static_cast<int>(to) - static_cast<int>(from);
        if (delta == +16) {
            st.en_passant = from + 8;
            st.hash ^= hashes::en_passant(st.en_passant);
            st.half_move = 0;
        } else if (delta == -16) {
            st.en_passant = from - 8;
            st.hash ^= hashes::en_passant(st.en_passant);
            st.half_move = 0;
        } else if (!is_promotion && captured == NO_PIECE) {
            // quiet pawn move (non-2-step) resets half-move
            st.half_move = 0;
        }
    }

    // Half-move clock for non-pawn quiets
    if (!moving_is_pawn && captured == NO_PIECE && !is_promotion) {
        st.half_move++;
    }

    // Full-move, side to move, last move, captured piece
    full_move += (side == WHITE);
    st.captured = captured;
    st.hash ^= hashes::side();
    st.last_move = move;

    // Switch side
    side = ~side;

    // Recompute checkers/snipers/blockers (kept as in your code)
    const square ksq = by(side, KING).front();
    st.checkers = attackers(ksq) & by(~side);
    for (side_e s : {WHITE, BLACK}) {
        auto [sn, bl] = find_snipers_and_blockers(s);
        st.snipers[s] = sn;
        st.blockers[s] = bl;
    }

    st.repetition = 1 + find_repetitions(st.hash, st.half_move);

    // Commit new state
    states.push_back(st);
}

inline uint8_t position_t::find_repetitions(hash_t hash, uint8_t half_move) const noexcept {
    int s = states.size();
    int limit = std::max(0, s - 2 * half_move);
    for (int i = s - 2; i >= limit; i -= 2) {
        if (states[i].hash == hash)
            return states[i].repetition;
    }
    return 0;
}

inline void position_t::undo_move(move_t move) noexcept {
    // Current state after the move
    const state_t& cur = states.back();
    // State before the move (exists because we always push on make_move)
    const state_t& prev = states[states.size() - 2];

    // Restore side/full-move first
    side = ~side;
    full_move -= (side == WHITE);

    const square from = move.from();
    const square to   = move.to();

    // Piece currently on 'to' (could be promoted piece)
    const piece final_piece    = board[to];
    const piece captured_piece = cur.captured;
    const bool  is_promotion   = (move.promotion() != NO_TYPE);

    // Original moving piece (promotion -> pawn)
    piece moving_piece = final_piece;
    if (is_promotion) moving_piece = piece{side, PAWN};

    // Detect en-passant via previous state's ep-square
    const bool is_en_passant = (moving_piece.type() == PAWN) &&
                               (prev.en_passant != NO_SQUARE) &&
                               (to == prev.en_passant) &&
                               (captured_piece != NO_PIECE) &&
                               (captured_piece.type() == PAWN);

    // Fast path: en-passant undo
    if (is_en_passant) {
        const square victim_sq = (side == WHITE) ? to - 8 : to + 8;

        // Board
        board[from]      = moving_piece;     // pawn back
        board[to]        = NO_PIECE;         // ep target square was empty before
        board[victim_sq] = captured_piece;   // restore captured pawn

        // Occupancy (mover)
        occupied_by_type[PAWN].reset(to);
        occupied_by_side[side].reset(to);
        occupied_by_type[PAWN].set(from);
        occupied_by_side[side].set(from);

        // Occupancy (victim)
        occupied_by_type[PAWN].set(victim_sq);
        occupied_by_side[~side].set(victim_sq);

        // Material: unchanged for EP (already handled in make_move)
        states.pop_back();
        return;
    }

    // Promotion undo
    if (is_promotion) {
        // Board: promoted piece -> captured (or empty), pawn back to 'from'
        board[from] = moving_piece;           // pawn
        board[to]   = captured_piece;         // restore captured (or empty)

        // Occupancy (remove promoted at 'to', add pawn at 'from')
        occupied_by_type[final_piece.type()].reset(to);
        occupied_by_side[side].reset(to);
        occupied_by_type[PAWN].set(from);
        occupied_by_side[side].set(from);

        // Restore captured occupancy/material if any
        if (captured_piece != NO_PIECE) {
            occupied_by_type[captured_piece.type()].set(to);
            occupied_by_side[captured_piece.side()].set(to);
            material[~side] += type_values[captured_piece.type()];
        }

        // Material delta (remove promotion gain)
        material[side] -= (type_values[final_piece.type()] - type_values[PAWN]);

        states.pop_back();
        return;
    }

    // Normal move (includes normal captures and castling)
    {
        // Board
        board[from] = moving_piece;
        board[to]   = captured_piece;

        // Occupancy for mover
        const type_e mt = moving_piece.type();
        occupied_by_type[mt].reset(to);
        occupied_by_type[mt].set(from);
        occupied_by_side[side].reset(to);
        occupied_by_side[side].set(from);

        // Restore captured piece occupancy/material
        if (captured_piece != NO_PIECE) {
            occupied_by_type[captured_piece.type()].set(to);
            occupied_by_side[captured_piece.side()].set(to);
            material[~side] += type_values[captured_piece.type()];
        }

        // Castling rook rollback (detect by king moving E->G/C)
        if (mt == KING) {
            if (from == E1 && to == G1) { // White O-O
                board[H1] = WROOK; board[F1] = NO_PIECE;
                occupied_by_type[ROOK].flip("f1h1"_b);
                occupied_by_side[WHITE].flip("f1h1"_b);
            } else if (from == E1 && to == C1) { // White O-O-O
                board[A1] = WROOK; board[D1] = NO_PIECE;
                occupied_by_type[ROOK].flip("a1d1"_b);
                occupied_by_side[WHITE].flip("d1a1"_b);
            } else if (from == E8 && to == G8) { // Black O-O
                board[H8] = BROOK; board[F8] = NO_PIECE;
                occupied_by_type[ROOK].flip("f8h8"_b);
                occupied_by_side[BLACK].flip("f8h8"_b);
            } else if (from == E8 && to == C8) { // Black O-O-O
                board[A8] = BROOK; board[D8] = NO_PIECE;
                occupied_by_type[ROOK].flip("a8d8"_b);
                occupied_by_side[BLACK].flip("d8a8"_b);
            }
        }
    }

    // Pop state last (restores hash, castle, ep, clocks, checkers, pins, repetition)
    states.pop_back();
}

inline bool position_t::can_null_move() const noexcept {
    bitboard non_pawn_pieces = by(side, KNIGHT, BISHOP) | by(side, ROOK, QUEEN);
    return !is_check() && last_move() != move_t{} && non_pawn_pieces.size() > 1 && by(side).size() > 4 && by(~side).size() > 4;
}

inline void position_t::make_null_move() noexcept {
    state_t new_state = states.back();

    new_state.hash ^= hashes::side();
    new_state.hash ^= hashes::en_passant(new_state.en_passant);
    new_state.en_passant = NO_SQUARE;
    new_state.captured = NO_PIECE;
    new_state.half_move++;
    new_state.last_move = move_t{};

    side = ~side;
    full_move += side == WHITE;

    auto king_square = by(side, KING).front();
    new_state.checkers = attackers(king_square) & by(~side);

    new_state.repetition = 1 + find_repetitions(new_state.hash, new_state.half_move);
    states.push_back(new_state);
}

inline void position_t::undo_null_move() noexcept {
    side = ~side;
    full_move -= side == WHITE;

    states.pop_back();
}

inline int16_t position_t::see(move_t move) const noexcept {

    auto find_least_piece = [this](bitboard board) -> square {
        for (type_e type : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
            bitboard pieces = board & by(type);
            if (!pieces.empty()) {
                return pieces.front();
            }
        }
        return NO_SQUARE;
    };

    bitboard rook_sliders = bitboards::rook_queen(move.to(), 0ull);
    bitboard bishop_sliders = bitboards::bishop_queen(move.to(), 0ull);
    bitboard occupied = by();
    bitboard attackers = this->attackers(move.to());
    bitboard blockers = this->states.back().blockers[WHITE] | this->states.back().blockers[BLACK];
    attackers.reset(move.from());
    occupied.reset(move.from());

    auto expand_attackers = [&](square from) {
        switch (board[from].type()) {
            case PAWN:
                if (from.file() == move.to().file()) {
                    attackers |= bitboards::rook_queen(from, occupied) & rook_sliders & by(ROOK, QUEEN) & occupied;
                } else {
                    attackers |= bitboards::bishop_queen(from, occupied) & bishop_sliders & by(BISHOP, QUEEN) & occupied;
                }
                break;
            case BISHOP:
                attackers |= bitboards::bishop_queen(from, occupied) & bishop_sliders & by(BISHOP, QUEEN) & occupied;
                break;
            case ROOK:
                attackers |= bitboards::rook_queen(from, occupied) & rook_sliders & by(ROOK, QUEEN) & occupied;
                break;
            case QUEEN:
                attackers |= bitboards::bishop_queen(from, occupied) & bishop_sliders & by(BISHOP, QUEEN) & occupied;
                attackers |= bitboards::rook_queen(from, occupied) & rook_sliders & by(ROOK, QUEEN) & occupied;
                break;
            default:
                break;
        }

        attackers &= ~blockers;
    };

    // std::println("attackers: {}", attackers);
    expand_attackers(move.from());

    // std::println("attackers: {}", attackers);

    int gain[32];
    int depth = 1;

    gain[0] = type_values[board[move.to()].type()];

    type_e attacked_piece_type = board[move.from()].type();

    if (move.promotion() != NO_TYPE) {
        gain[0] += type_values[move.promotion()] - type_values[PAWN];
        attacked_piece_type = move.promotion();
    }
    
    bitboard en_passant = states.back().en_passant;
    if (!en_passant.empty() && en_passant.front() == move.to()) {
        gain[0] += type_values[PAWN];
        attacked_piece_type = PAWN;
    }

    side_e side = ~this->side;
    
    // std::println("in");

    while (!attackers.empty()) {
        bitboard current = attackers & by(side);

        if (current.empty()) break;

        square least_sq = find_least_piece(current);

        gain[depth] = type_values[attacked_piece_type] - gain[depth-1];

        attacked_piece_type = board[least_sq].type();
        attackers.reset(least_sq);
        occupied.reset(least_sq);

        if (attacked_piece_type == PAWN && bitboard{move.to()} & "18"_r) {
            gain[depth] += type_values[QUEEN] - type_values[PAWN];
            attacked_piece_type = QUEEN;
        }

        expand_attackers(least_sq);

        side = ~side;
        depth++;
    }

    while (--depth)
        gain[depth-1]= -std::max(-gain[depth-1], gain[depth]);

    // std::println("out");

    return gain[0];
}
