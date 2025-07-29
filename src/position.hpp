#pragma once

#include "bitboard.hpp"
#include "bitboards.hpp"
#include "piece.hpp"
#include "hashes.hpp"
#include <vector>
#include <print>

struct move2_t {
    constexpr move2_t() noexcept : from_{NO_SQUARE}, to_{NO_SQUARE}, promotion_{NO_TYPE} {
    }

    constexpr move2_t(square from, square to, type_e promotion = NO_TYPE) noexcept : from_{from}, to_{to}, promotion_{promotion} {
    }

    constexpr move2_t(std::string_view string) noexcept : from_{string.substr(0, 2)}, to_{string.substr(2, 2)} {
        string.remove_prefix(4);
        promotion_ = operator""_t(string.data(), string.size());
    }

    constexpr bool operator==(const move2_t& other) const noexcept = default;

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

inline constexpr move2_t operator""_m(const char* str, size_t len) noexcept {
    return move2_t{std::string_view{str, len}};
}


template <>
struct std::formatter<move2_t> {
    constexpr auto parse(std::format_parse_context& ctx){
        return ctx.begin();
    }

    auto format(move2_t move, std::format_context& ctx) const {
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

// use once per thread
struct position_t {

    enum {
        MAX_MOVES_PER_PLY = 218,
        MAX_MOVES_PER_GAME = 1024
    };

    // constexpr static size_t MAX_MOVES_PER_PLY = 218;
    // constexpr static size_t MAX_MOVES_PER_GAME = 1024;

    // use once per ply
    struct state_t {
        bitboard castle;
        bitboard en_passant;
        bitboard checkers;
        hash_t hash;
        uint8_t half_move;
        piece captured;    
        bool null_move;

        bool operator==(const state_t& other) const noexcept = default;
    };

    position_t() noexcept;
    position_t(std::string_view fen) noexcept;

    bool operator==(const position_t& other) const noexcept = default;

    void make_move(move2_t move) noexcept;
    void undo_move(move2_t move) noexcept;

    bool can_null_move() const noexcept;
    void make_null_move() noexcept;
    void undo_null_move() noexcept;

    bitboard attackers(side_e side) const noexcept;
    bitboard attackers(square square) const noexcept;
    bitboard attackers(square square, side_e side) const noexcept;

    std::span<move2_t> generate_moves(std::span<move2_t, MAX_MOVES_PER_PLY> buffer, bitboard valid_targets) const noexcept;

    int see(const move2_t& move) const noexcept;

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

    // uint16_t get_half_move() const noexcept {
    //     return states.back().half_move;
    // }

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

    bool is_check() const noexcept {
        return !states.back().checkers.empty();
    }

    bool is_50_moves_rule() const noexcept {
        return states.back().half_move >= 100;
    }

    bool is_3_fold_repetition() const noexcept {
        // 3-fold can only happen within last 50 moves (50-move rule)
        // auto range = std::span{states}.last(std::min(states.size(), 50ul));
        return std::ranges::count(states, hash(), &state_t::hash) >= 3;
    }

    bool is_no_material() const noexcept {
        return material[WHITE] <= 300 && by(WPAWN).empty() && material[BLACK] <= 300 && by(BPAWN).empty();
    }

// private:
    piece board[64];
    bitboard occupied_by_type[7];
    bitboard occupied_by_side[2];
    int16_t material[2];
    uint16_t full_move;
    side_e side;
    std::vector<state_t> states;
};

inline position_t::position_t() noexcept : position_t{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv} {
}

inline position_t::position_t(std::string_view fen) noexcept : board{}, occupied_by_type{}, occupied_by_side{}, material{}, full_move{}, side{WHITE}, states{} {
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
        new_state.en_passant = bitboard{en_passant_part};
        new_state.hash ^= hashes::en_passant(new_state.en_passant);
    }

    if (fen_part != fen_parts.end()) {
        std::string_view half_move_part {*fen_part++};
        std::from_chars(half_move_part.begin(), half_move_part.end(), new_state.half_move);
    }

    if (fen_part != fen_parts.end()) {
        std::string_view full_move_part {*fen_part++};
        std::from_chars(full_move_part.begin(), full_move_part.end(), full_move);
    }

    auto king_square = by(side, KING).front();
    new_state.checkers = attackers(king_square) & by(~side);

    new_state.captured = NO_PIECE;
    new_state.null_move = false;

    states.reserve(MAX_MOVES_PER_GAME);
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

inline std::span<move2_t> position_t::generate_moves(std::span<move2_t, MAX_MOVES_PER_PLY> buffer, bitboard valid_targets) const noexcept {

    size_t index = 0;
    bitboard checkers = states.back().checkers;
    bitboard en_passant = states.back().en_passant;
    bitboard attacked = attackers(~side);
    bitboard valid_en_passant = 0ull;
    square ksq = by(side, KING).front();

    // std::println("attacked:\n{:b}", attacked);

    for (square to_square : bitboards::king(ksq) & ~attacked & ~by(side) & valid_targets) {
        buffer[index++] = {ksq, to_square};
    }

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


    bitboard pinned = 0ull;
    bitboard valid_for_pinned[64];
    std::ranges::fill(valid_for_pinned, ~0ull);

    bitboard ksr = bitboards::rook_queen(ksq, by()) & by(side);
    bitboard ksb = bitboards::bishop_queen(ksq, by()) & by(side);
    for (square sq : bitboards::rook_queen(ksq, 0ull) & by(~side, ROOK, QUEEN)) {
        bitboard rsk = bitboards::rook_queen(sq, by()) & by(side);
        bitboard pin = ksr & rsk;
        pinned |= pin;
        if (pin)
            valid_for_pinned[pin.front()] = bitboards::line(ksq, sq);
    }
    for (square sq : bitboards::bishop_queen(ksq, 0ull) & by(~side, BISHOP, QUEEN)) {
        bitboard bsk = bitboards::bishop_queen(sq, by()) & by(side);
        bitboard pin = ksb & bsk;
        pinned |= pin;
        if (pin)
            valid_for_pinned[pin.front()] = bitboards::line(ksq, sq);
    }


    valid_targets &= ~(by(side) | by(KING));

    for (square from_square : by(side, KNIGHT) & ~pinned) {
        for (square to_square : bitboards::knight(from_square) & valid_targets) {
            buffer[index++] = {from_square, to_square};
        }
    }

    for (square from_square : by(side, ROOK, QUEEN)) {
        for (square to_square : bitboards::rook_queen(from_square, by()) & valid_targets & valid_for_pinned[from_square]) {
            buffer[index++] = {from_square, to_square};
        }
    }

    for (square from_square : by(side, BISHOP, QUEEN)) {
        for (square to_square : bitboards::bishop_queen(from_square, by()) & valid_targets & valid_for_pinned[from_square]) {
            buffer[index++] = {from_square, to_square};
        }
    }   

    const auto generate_normal = [&](bitboard targets, int delta) {
        for (square to : targets)
            if (valid_for_pinned[to + delta] & bitboard{to})
                buffer[index++] = {to + delta, to};
    };

    const auto generate_promotion = [&](bitboard targets, int delta) {
        for (square to : targets)
            if (valid_for_pinned[to + delta] & bitboard{to})
                for (type_e type : {QUEEN, ROOK, BISHOP, KNIGHT})
                    buffer[index++] = {to + delta, to, type};
    };

    bitboard pawns = by(side, PAWN);

    if (side == WHITE) {
        bitboard push = pawns << 8 & ~by();
        bitboard targets;

        targets = push & valid_targets;
        generate_normal(targets & ~"8"_r, -8);
        generate_promotion(targets & "8"_r, -8);

        targets = push << 8 & ~by() & "4"_r & valid_targets;
        generate_normal(targets, -16);

        targets = pawns << 7 & ~"h"_f & by(~side) & valid_targets;
        generate_normal(targets & ~"8"_r, -7);
        generate_promotion(targets & "8"_r, -7);

        targets = pawns << 9 & ~"a"_f & by(~side) & valid_targets;
        generate_normal(targets & ~"8"_r, -9);
        generate_promotion(targets & "8"_r, -9);
    } else {
        bitboard push = pawns >> 8 & ~by();
        bitboard targets;

        targets = push & valid_targets;
        generate_normal(targets & ~"1"_r, +8);
        generate_promotion(targets & "1"_r, +8);

        targets = push >> 8 & ~by() & "5"_r & valid_targets;
        generate_normal(targets, +16);

        targets = pawns >> 7 & ~"a"_f & by(~side) & valid_targets;
        generate_normal(targets & ~"1"_r, +7);
        generate_promotion(targets & "1"_r, +7);

        targets = pawns >> 9 & ~"h"_f & by(~side) & valid_targets;
        generate_normal(targets & ~"1"_r, +9);
        generate_promotion(targets & "1"_r, +9);
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

inline void position_t::make_move(move2_t move) noexcept {
    state_t new_state = states.back();

    square from_square = move.from();
    square to_square = move.to();
    piece moving_piece = board[from_square];


    if (moving_piece.type() == PAWN && new_state.en_passant == bitboard{to_square}) {
        square victim_square = side == WHITE ? to_square - 8 : to_square + 8;
        std::swap(board[to_square], board[victim_square]);
        occupied_by_type[PAWN].reset(victim_square);
        occupied_by_type[PAWN].set(to_square);
        occupied_by_side[~side].reset(victim_square);
        occupied_by_side[~side].set(to_square);
        new_state.hash ^= hashes::hash(piece{~side, PAWN}, victim_square);
        new_state.hash ^= hashes::hash(piece{~side, PAWN}, to_square);
    }


    piece captured_piece = board[to_square];
    
    // ✅ Handle promotion
    piece final_piece = moving_piece;
    if (move.promotion() != NO_TYPE) {
        final_piece = piece{side, move.promotion()};
    }

    // ✅ Update board with correct piece
    board[to_square] = final_piece;
    board[from_square] = NO_PIECE;

    new_state.half_move++;
    new_state.hash ^= hashes::en_passant(new_state.en_passant);
    new_state.en_passant = 0ll;

    // ✅ Handle capture (including promotion captures)
    if (captured_piece != NO_PIECE) {
        // std::println("before:{}{}{}{}{}{}{}", occupied_by_type[0], occupied_by_type[1], occupied_by_type[2], occupied_by_type[3], occupied_by_type[4], occupied_by_type[5], occupied_by_type[6]);
        occupied_by_type[captured_piece.type()].reset(to_square);
        // std::println("after:{}{}{}{}{}{}{}", occupied_by_type[0], occupied_by_type[1], occupied_by_type[2], occupied_by_type[3], occupied_by_type[4], occupied_by_type[5], occupied_by_type[6]);
        occupied_by_side[captured_piece.side()].reset(to_square);
        new_state.hash ^= hashes::hash(captured_piece, to_square);
        material[~side] -= type_values[captured_piece.type()];
        new_state.half_move = 0;
        new_state.hash ^= hashes::castle(new_state.castle);
        new_state.castle.reset(to_square);
        new_state.hash ^= hashes::castle(new_state.castle);
    }

    // ✅ Update occupied bitboards for promotion
    if (move.promotion() != NO_TYPE) {
        // Remove pawn
        occupied_by_type[moving_piece.type()].reset(from_square);
        occupied_by_side[moving_piece.side()].reset(from_square);
        new_state.hash ^= hashes::hash(moving_piece, from_square);
        
        // Add promoted piece
        occupied_by_type[final_piece.type()].set(to_square);
        occupied_by_side[final_piece.side()].set(to_square);
        new_state.hash ^= hashes::hash(final_piece, to_square);
        
        // Update material count
        material[side] += type_values[final_piece.type()] - type_values[moving_piece.type()];
    } else {
        // Normal move
        occupied_by_type[moving_piece.type()].set(to_square);
        occupied_by_type[moving_piece.type()].reset(from_square);
        occupied_by_side[moving_piece.side()].set(to_square);
        occupied_by_side[moving_piece.side()].reset(from_square);
        new_state.hash ^= hashes::hash(moving_piece, from_square);
        new_state.hash ^= hashes::hash(final_piece, to_square);
    }

    // Castle handling (unchanged)
    switch (moving_piece) {
        case WKING:
            if (from_square == E1 && to_square == G1) {
                occupied_by_type[ROOK].flip("f1h1"_b);
                occupied_by_side[WHITE].flip("f1h1"_b);
                std::swap(board[F1], board[H1]);
                new_state.hash ^= hashes::hash(WROOK, F1) ^ hashes::hash(WROOK, H1);
            } else if (from_square == E1 && to_square == C1) {
                occupied_by_type[ROOK].flip("a1d1"_b);
                occupied_by_side[WHITE].flip("d1a1"_b);
                std::swap(board[A1], board[D1]);
                new_state.hash ^= hashes::hash(WROOK, A1) ^ hashes::hash(WROOK, D1);
            }
            new_state.castle.reset("a1h1"_b);
            new_state.hash ^= hashes::castle(states.back().castle);
            new_state.hash ^= hashes::castle(new_state.castle);
            break;
        case BKING:
            if (from_square == E8 && to_square == G8) {
                occupied_by_type[ROOK].flip("f8h8"_b);
                occupied_by_side[BLACK].flip("f8h8"_b);
                std::swap(board[F8], board[H8]);
                new_state.hash ^= hashes::hash(BROOK, F8) ^ hashes::hash(BROOK, H8);
            } else if (from_square == E8 && to_square == C8) {
                occupied_by_type[ROOK].flip("a8d8"_b);
                occupied_by_side[BLACK].flip("d8a8"_b);
                std::swap(board[A8], board[D8]);
                new_state.hash ^= hashes::hash(BROOK, A8) ^ hashes::hash(BROOK, D8);
            }
            new_state.castle.reset("a8h8"_b);
            new_state.hash ^= hashes::castle(states.back().castle);
            new_state.hash ^= hashes::castle(new_state.castle);
            break;
        case WROOK:
            new_state.castle.reset("a1h1"_b & bitboard{from_square});
            new_state.hash ^= hashes::castle(states.back().castle);
            new_state.hash ^= hashes::castle(new_state.castle);
            break;
        case BROOK:
            new_state.castle.reset("a8h8"_b & bitboard{from_square});
            new_state.hash ^= hashes::castle(states.back().castle);
            new_state.hash ^= hashes::castle(new_state.castle);
            break;
        case WPAWN:
            if (to_square - from_square == +16) {
                new_state.en_passant = bitboard{from_square + 8};
                new_state.hash ^= hashes::en_passant(new_state.en_passant);
            }
            new_state.half_move = 0;
            break;
        case BPAWN:
            if (to_square - from_square == -16) {
                new_state.en_passant = bitboard{from_square - 8};
                new_state.hash ^= hashes::en_passant(new_state.en_passant);
            }
            new_state.half_move = 0;
            break;
        default:
            break;
    }

    full_move += side == WHITE;
    new_state.captured = captured_piece;
    new_state.hash ^= hashes::side();
    new_state.null_move = false;

    side = ~side;

    auto king_square = by(side, KING).front();
    new_state.checkers = attackers(king_square) & by(~side);
    // auto king_square = by(side, KING).front();
    // new_state.checkers = attackers(king_square, ~side) & by(~side);

    states.push_back(new_state);
}

inline void position_t::undo_move(move2_t move) noexcept {
    side = ~side;

    square from_square = move.from();
    square to_square = move.to();
    piece final_piece = board[to_square];  // This might be promoted piece
    piece captured_piece = states.back().captured;

    // ✅ Determine original moving piece
    piece moving_piece = final_piece;
    if (move.promotion() != NO_TYPE) {
        // If it was a promotion, original piece was a pawn
        moving_piece = piece{side, PAWN};
    }

    // ✅ Restore board
    board[from_square] = moving_piece;  // Original piece goes back
    board[to_square] = captured_piece;  // Captured piece restored (or NO_PIECE)

    // ✅ Update occupied bitboards for promotion
    if (move.promotion() != NO_TYPE) {
        // Remove promoted piece
        occupied_by_type[final_piece.type()].reset(to_square);
        occupied_by_side[final_piece.side()].reset(to_square);
        
        // Restore original pawn
        occupied_by_type[moving_piece.type()].set(from_square);
        occupied_by_side[moving_piece.side()].set(from_square);
        
        // Update material count
        material[side] -= type_values[final_piece.type()] - type_values[moving_piece.type()];
    } else {
        // Normal move undo
        occupied_by_type[moving_piece.type()].flip(to_square);
        occupied_by_type[moving_piece.type()].flip(from_square);
        occupied_by_side[moving_piece.side()].flip(to_square);
        occupied_by_side[moving_piece.side()].flip(from_square);
    }

    // ✅ Restore captured piece
    if (captured_piece != NO_PIECE) {
        occupied_by_type[captured_piece.type()].set(to_square);
        occupied_by_side[captured_piece.side()].set(to_square);
        material[~side] += type_values[captured_piece.type()];
    }

    // Castle handling (unchanged)
    switch (moving_piece) {
        case WKING:
            if (from_square == E1 && to_square == G1) {
                occupied_by_type[ROOK].flip("f1h1"_b);
                occupied_by_side[WHITE].flip("f1h1"_b);
                std::swap(board[F1], board[H1]);
            } else if (from_square == E1 && to_square == C1) {
                occupied_by_type[ROOK].flip("a1d1"_b);
                occupied_by_side[WHITE].flip("d1a1"_b);
                std::swap(board[A1], board[D1]);
            }
            break;
        case BKING:
            if (from_square == E8 && to_square == G8) {
                occupied_by_type[ROOK].flip("f8h8"_b);
                occupied_by_side[BLACK].flip("f8h8"_b);
                std::swap(board[F8], board[H8]);
            } else if (from_square == E8 && to_square == C8) {
                occupied_by_type[ROOK].flip("a8d8"_b);
                occupied_by_side[BLACK].flip("d8a8"_b);
                std::swap(board[A8], board[D8]);
            }
            break;
        default:
            break;
    }

    full_move -= side == WHITE;
    states.pop_back();

    if (moving_piece.type() == PAWN && states.back().en_passant == bitboard{to_square}) {
        square victim_square = side == WHITE ? to_square - 8 : to_square + 8;
        std::swap(board[to_square], board[victim_square]);
        occupied_by_type[PAWN].set(victim_square);
        occupied_by_type[PAWN].reset(to_square);
        occupied_by_side[~side].set(victim_square);
        occupied_by_side[~side].reset(to_square);
    }
}

inline bool position_t::can_null_move() const noexcept {
    bitboard non_pawn_pieces = by(side, KNIGHT, BISHOP) | by(side, ROOK, QUEEN);
    return !is_check() && !states.back().null_move && !non_pawn_pieces.empty();
}


inline void position_t::make_null_move() noexcept {
    state_t new_state = states.back();

    new_state.hash ^= hashes::side();
    new_state.hash ^= hashes::en_passant(new_state.en_passant);
    new_state.en_passant = 0ull;
    new_state.captured = NO_PIECE;
    new_state.half_move++;
    new_state.null_move = true;

    side = ~side;
    full_move += side == WHITE;

    auto king_square = by(side, KING).front();
    new_state.checkers = attackers(king_square) & by(~side);

    states.push_back(new_state);
}

inline void position_t::undo_null_move() noexcept {
    side = ~side;
    full_move -= side == WHITE;

    states.pop_back();
}

inline int position_t::see(const move2_t& move) const noexcept {

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
    attackers.reset(move.from());
    occupied.reset(move.from());

    // std::println("attackers: {}", attackers);

    switch (board[move.from()].type()) {
    case BISHOP:
    case PAWN:
        attackers |= bitboards::bishop_queen(move.from(), occupied) & bishop_sliders & by(BISHOP, QUEEN) & occupied;
        break;
    case ROOK:
        attackers |= bitboards::rook_queen(move.from(), occupied) & rook_sliders & by(ROOK, QUEEN) & occupied;
        break;
    case QUEEN:
        attackers |= bitboards::bishop_queen(move.from(), occupied) & bishop_sliders & by(BISHOP, QUEEN) & occupied;
        attackers |= bitboards::rook_queen(move.from(), occupied) & rook_sliders & by(ROOK, QUEEN) & occupied;
        break;
    default:
        break;
    }

    // std::println("attackers: {}", attackers);

    int gain[32];
    int depth = 1;

    gain[0] = type_values[board[move.to()].type()];

    int attacked_piece_value = type_values[board[move.from()].type()];

    if (move.promotion() != NO_TYPE) {
        gain[0] += type_values[move.promotion()] - type_values[PAWN];
        attacked_piece_value = type_values[move.promotion()];
    }

    // switch (move.promotion()) {
    // case QUEEN:
    //     gain[0] += type_values[QUEEN] - type_values[PAWN];
    //     attacked_piece_value = type_values[QUEEN];
    //     break;
    // case ROOK:
    //     gain[0] += type_values[ROOK] - type_values[PAWN];
    //     attacked_piece_value = type_values[ROOK];
    //     break;
    // case BISHOP:
    //     gain[0] += type_values[BISHOP] - type_values[PAWN];
    //     attacked_piece_value = type_values[BISHOP];
    //     break;
    // case KNIGHT:
    //     gain[0] += type_values[KNIGHT] - type_values[PAWN];
    //     attacked_piece_value = type_values[KNIGHT];
    //     break;
    // // case move_t::EN_PASSANT:
    // //     gain[0] += piece_values[PAWN];
    // //     break;
    // default:
    //     break;
    // }

    side_e side = ~this->side;
    
    // std::println("in");

    while (!attackers.empty()) {
        bitboard current = attackers & by(side);

        if (current.empty()) break;

        square least_sq = find_least_piece(current);

        gain[depth] = attacked_piece_value - gain[depth-1];

        attacked_piece_value = type_values[board[least_sq].type()];
        attackers.reset(least_sq);
        occupied.reset(least_sq);

        switch (board[least_sq].type()) {
            case BISHOP:
                attackers |= bitboards::bishop_queen(least_sq, occupied) & bishop_sliders & by(BISHOP, QUEEN) & occupied;
                break;
            case ROOK:
                attackers |= bitboards::rook_queen(least_sq, occupied) & rook_sliders & by(ROOK, QUEEN) & occupied;
                break;
            case QUEEN:
                attackers |= bitboards::bishop_queen(least_sq, occupied) & bishop_sliders & by(BISHOP, QUEEN) & occupied;
                attackers |= bitboards::rook_queen(least_sq, occupied) & rook_sliders & by(ROOK, QUEEN) & occupied;
                break;
            default:
                break;
        }

        side = ~side;
        depth++;
    }

    while (--depth)
        gain[depth-1]= -std::max(-gain[depth-1], gain[depth]);

    // std::println("out");

    return gain[0];
}
