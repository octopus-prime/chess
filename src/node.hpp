#pragma once

#include "square.hpp"
#include "piece.hpp"
#include "side.hpp"
#include "bitboard.hpp"
#include "bitboards.hpp"
#include "hashes.hpp"
#include "move.hpp"
// #include "nnue.hpp"
#include "dirty_piece.hpp"
#include "nnue/nnue.hpp"

class node
{
    using NNUE = nnue::big_nnue;

    // piece board[64];
    bitboard occupied_by_side[SIDE_MAX];
    bitboard occupied_by_type[TYPE_MAX];
    bitboard castle;
    bitboard en_passant;
    uint64_t hash_;
    const node* parent_;
    const move_t* move_;
    size_t dirty_pieces_size;
    dirty_piece dirty_pieces_data[3];
    size_t rule50;
    mutable NNUE::Accumulator accumulator;

public:
    // NNUEdata nnue;

    enum generation_t {all, captures};

    constexpr node() noexcept
        : castle{0}, en_passant{0}, hash_{0}, parent_{nullptr}, move_{nullptr}, dirty_pieces_size{0}, rule50{0}
    {
    }

    constexpr node(std::span<const bitboard, SIDE_MAX> occupied_by_side, std::span<const bitboard, TYPE_MAX> occupied_by_type, bitboard castle, bitboard en_passant) noexcept
    : castle{castle}, en_passant{en_passant}, parent_{nullptr}, move_{nullptr}, dirty_pieces_size{0}
        {
            std::ranges::copy(occupied_by_side, this->occupied_by_side);
            std::ranges::copy(occupied_by_type, this->occupied_by_type);
        }

    node(std::string_view fen, side_e& side);

    constexpr node(const node& parent) noexcept
        : castle{parent.castle}, en_passant{parent.en_passant}, hash_{parent.hash_}, parent_(&parent), move_(nullptr), dirty_pieces_size{0}, rule50{parent.rule50 + 1}
        {
            std::ranges::copy(parent.occupied_by_side, this->occupied_by_side);
            std::ranges::copy(parent.occupied_by_type, this->occupied_by_type);
        }

    constexpr const node* parent() const noexcept {
        return parent_;
    }

    constexpr const move_t* moved() const noexcept {
        return move_;
    }

    constexpr bitboard occupied() const noexcept {
        return occupied_by_side[WHITE] | occupied_by_side[BLACK];
    }

    template <side_e side>
    constexpr bitboard occupied() const noexcept {
        return occupied_by_side[side];
    }

    template <side_e side>
    constexpr bitboard king() const noexcept {
        return occupied_by_type[KING] & occupied_by_side[side];
    }

    template <side_e side>
    constexpr bitboard rook_queen() const noexcept {
        return (occupied_by_type[ROOK] | occupied_by_type[QUEEN]) & occupied_by_side[side];
    }

    template <side_e side>
    constexpr bitboard bishop_queen() const noexcept {
        return (occupied_by_type[BISHOP] | occupied_by_type[QUEEN]) & occupied_by_side[side];
    }

    constexpr bitboard queen() const noexcept {
        return occupied_by_type[QUEEN];
    }

    template <side_e side>
    constexpr bitboard queen() const noexcept {
        return occupied_by_type[QUEEN] & occupied_by_side[side];
    }

    template <side_e side>
    constexpr bitboard rook() const noexcept {
        return occupied_by_type[ROOK] & occupied_by_side[side];
    }

    template <side_e side>
    constexpr bitboard bishop() const noexcept {
        return occupied_by_type[BISHOP] & occupied_by_side[side];
    }

    template <side_e side>
    constexpr bitboard knight() const noexcept {
        return occupied_by_type[KNIGHT] & occupied_by_side[side];
    }

    constexpr bitboard pawn() const noexcept {
        return occupied_by_type[PAWN];
    }

    template <side_e side>
    constexpr bitboard pawn() const noexcept {
        return occupied_by_type[PAWN] & occupied_by_side[side];
    }

    constexpr bitboard board(type_e type) const noexcept {
        return occupied_by_type[type];
    }

    constexpr bitboard board(piece piece) const noexcept {
        return occupied_by_type[piece.type()] & occupied_by_side[piece.side()];
    }

    template <side_e side>
    constexpr uint64_t hash() const noexcept {
        // return hash_;
        // auto c = hashes::castle(castle);
        // auto e = hashes::en_passant(en_passant);
        auto s = hashes::color(side);
        return hash_ ^ uint64_t{castle} ^ uint64_t{en_passant} ^ uint64_t{s};
    }

    constexpr auto get_dirty_pieces() const noexcept {
        return std::span{dirty_pieces_data}.first(dirty_pieces_size);
    }

    constexpr size_t get_rule50() const noexcept {
        return rule50;
    }

    constexpr auto& get_accumulator() const noexcept {
        return accumulator;
    }

    template <side_e side>
    /*constexpr*/ bitboard attackers() const noexcept;

    template <side_e side>
    bitboard attackers(square square) const noexcept;

    template <side_e side>
    bitboard checkers() const noexcept;

    template <side_e side, generation_t generation>
    std::span<move_t> generate(std::span<move_t, 256> moves) const noexcept;

    template <side_e side>
    void execute(const move_t& move) noexcept;

    template <side_e side>
    void execute(std::string_view move);

    // template <side_t side>
    std::pair<bitboard, const move_t*> execute(bitboard en_passant, const move_t* move) noexcept {
        auto e = this->en_passant;
        auto m = this->move_;
        this->en_passant = en_passant;
        this->move_ = move;
        // accumulator.computed[0] = false;
        // accumulator.computed[1] = false;
        return {e, m};
    }

    template <side_e side>
    constexpr int material() const noexcept;
};

template <side_e side>
/*constexpr*/ bitboard node::attackers() const noexcept {
  bitboard out = 0ull;
  out |= bitboards::king(king<side>());
  out |= bitboards::knight(knight<side>());
  out |= bitboards::slider(rook_queen<side>(), bishop_queen<side>(), occupied() & ~king<~side>());
  out |= bitboards::pawn<side>(pawn<side>());
  return out;
}

template <side_e side>
inline bitboard node::attackers(square square) const noexcept {
  bitboard out = 0ull;
  out |= bitboards::king(square) & king<side>();
  out |= bitboards::knight(square) & knight<side>();
  out |= bitboards::rook_queen(square, occupied()) & rook_queen<side>();
  out |= bitboards::bishop_queen(square, occupied()) & bishop_queen<side>();
  out |= bitboards::pawn<~side>(square) & pawn<side>();
  return out;
}

template <side_e side>
inline bitboard node::checkers() const noexcept {
  auto square = king<side>().front();
  return attackers<~side>(square);
}

template <side_e side>
constexpr int node::material() const noexcept {
    int score = 0;
    score += (queen<WHITE>().size() - queen<BLACK>().size()) * 900;
    score += (rook<WHITE>().size() - rook<BLACK>().size()) * 500;
    score += (bishop<WHITE>().size() - bishop<BLACK>().size()) * 350;
    score += (knight<WHITE>().size() - knight<BLACK>().size()) * 300;
    score += (pawn<WHITE>().size() - pawn<BLACK>().size()) * 100;
    return side == WHITE ? score : -score;
}

template <side_e side, node::generation_t generation>
std::span<move_t> node::generate(std::span<move_t, 256> moves) const noexcept
{
    int index = 0;

    bitboard attacked = this->attackers<~side>();
    bitboard checkers = this->checkers<side>();
    bitboard valids = generation == all ? bitboards::ALL : occupied<~side>();
    bitboard validse = 0ull;
    bitboard pinned = 0ull;
    bitboard valid_for_pinned[64];
    std::ranges::fill(valid_for_pinned, ~0ull);

    const auto generate_pinned = [&]() noexcept
    {
        square ksq = king<side>().front();
        bitboard ksr = bitboards::rook_queen(ksq, occupied()) & occupied<side>();
        bitboard ksb = bitboards::bishop_queen(ksq, occupied()) & occupied<side>();
        bitboard bcr = bitboards::rook_queen(ksq, 0ull) & rook_queen<~side>();
        bitboard bcb = bitboards::bishop_queen(ksq, 0ull) & bishop_queen<~side>();
        for (square sq : bcr)
        {
            bitboard rsk = bitboards::rook_queen(sq, occupied()) & occupied<side>();
            bitboard pin = (ksr & rsk);
            pinned |= pin;
            if (pin)
                valid_for_pinned[pin.front()] = bitboards::line(ksq, sq);
        }
        for (square sq : bcb)
        {
            bitboard bsk = bitboards::bishop_queen(sq, occupied()) & occupied<side>();
            bitboard pin = (ksb & bsk);
            pinned |= pin;
            if (pin)
                valid_for_pinned[pin.front()] = bitboards::line(ksq, sq);
        }
    };

    const auto generate_king = [&]() noexcept
    {
        square from = king<side>().front();
        bitboard targets = bitboards::king(from) & ~occupied<side>() & ~attacked & valids;
        for (square to : targets)
            moves[index++] = {move_t::KING, from, to};
    };

    const auto generate_castle = [&]() noexcept
    {
        if constexpr (side == WHITE)
        {
            if ((castle & "h1"_b) && !(occupied() & "f1g1"_b) && !(attacked & "e1f1g1"_b))
                moves[index++] = {move_t::CASTLE_SHORT, "e1"_s, "g1"_s};
            if ((castle & "a1"_b) && !(occupied() & "b1c1d1"_b) && !(attacked & "e1d1c1"_b))
                moves[index++] = {move_t::CASTLE_LONG, "e1"_s, "c1"_s};
        }
        else
        {
            if ((castle & "h8"_b) && !(occupied() & "f8g8"_b) && !(attacked & "e8f8g8"_b))
                moves[index++] = {move_t::CASTLE_SHORT, "e8"_s, "g8"_s};
            if ((castle & "a8"_b) && !(occupied() & "b8c8d8"_b) && !(attacked & "e8d8c8"_b))
                moves[index++] = {move_t::CASTLE_LONG, "e8"_s, "c8"_s};
        }
    };

    const auto generate_knight = [&]() noexcept
    {
        bitboard sources = knight<side>() & ~pinned;
        for (square from : sources)
        {
            bitboard targets = bitboards::knight(from) & ~occupied<side>() & valids;
            for (square to : targets)
                moves[index++] = {move_t::KNIGHT, from, to};
        }
    };

    const auto generate_rook_queen = [&]() noexcept
    {
        constexpr move_t::type_t rook_or_queen[] = {move_t::ROOK, move_t::QUEEN};
        bitboard sources = rook_queen<side>();
        for (square from : sources)
        {
            auto type = rook_or_queen[bishop_queen<side>()[from]];
            bitboard targets = bitboards::rook_queen(from, occupied()) & ~occupied<side>() & valids & valid_for_pinned[from];
            for (square to : targets)
                moves[index++] = {type, from, to};
        }
    };

    const auto generate_bishop_queen = [&]() noexcept
    {
        constexpr move_t::type_t bishop_or_queen[] = {move_t::BISHOP, move_t::QUEEN};
        bitboard sources = bishop_queen<side>();
        for (square from : sources)
        {
            auto type = bishop_or_queen[rook_queen<side>()[from]];
            bitboard targets = bitboards::bishop_queen(from, occupied()) & ~occupied<side>() & valids & valid_for_pinned[from];
            for (square to : targets)
                moves[index++] = {type, from, to};
        }
    };

    const auto generate_pawn = [&]() noexcept
    {
        const auto generate_normal = [&](bitboard targets, int delta) noexcept
        {
            for (square to : targets)
                if (valid_for_pinned[to + delta] & bitboard{to})
                    moves[index++] = {move_t::PAWN, to + delta, to};
        };

        const auto generate_double_push = [&](bitboard targets, int delta) noexcept
        {
            for (square to : targets)
                if (valid_for_pinned[to + delta] & bitboard{to})
                    moves[index++] = {move_t::DOUBLE_PUSH, to + delta, to};
        };

        const auto generate_promotion = [&](bitboard targets, int delta) noexcept
        {
            for (square to : targets)
                if (valid_for_pinned[to + delta] & bitboard{to})
                    for (move_t::type_t type : {move_t::PROMOTE_QUEEN, move_t::PROMOTE_ROOK, move_t::PROMOTE_BISHOP, move_t::PROMOTE_KNIGHT})
                        moves[index++] = {type, to + delta, to};
        };

        const bitboard sources = pawn<side>(); // & ~pinned;
        bitboard targets;

        if constexpr (side == WHITE)
        {
            const bitboard push = sources << 8 & ~occupied();

            targets = push & valids;
            generate_normal(targets & ~"8"_r, -8);
            generate_promotion(targets & "8"_r, -8);

            targets = push << 8 & ~occupied() & "4"_r & valids;
            generate_double_push(targets, -16);

            targets = sources << 7 & ~"h"_f & occupied<~side>() & valids;
            generate_normal(targets & ~"8"_r, -7);
            generate_promotion(targets & "8"_r, -7);

            targets = sources << 9 & ~"a"_f & occupied<~side>() & valids;
            generate_normal(targets & ~"8"_r, -9);
            generate_promotion(targets & "8"_r, -9);
        }
        else
        {
            const bitboard push = sources >> 8 & ~occupied();

            targets = push & valids;
            generate_normal(targets & ~"1"_r, +8);
            generate_promotion(targets & "1"_r, +8);

            targets = push >> 8 & ~occupied() & "5"_r & valids;
            generate_double_push(targets, +16);

            targets = sources >> 7 & ~"a"_f & occupied<~side>() & valids;
            generate_normal(targets & ~"1"_r, +7);
            generate_promotion(targets & "1"_r, +7);

            targets = sources >> 9 & ~"h"_f & occupied<~side>() & valids;
            generate_normal(targets & ~"1"_r, +9);
            generate_promotion(targets & "1"_r, +9);
        }

        bitboard board = sources & bitboards::pawn<~side>(bitboard{en_passant & (valids | validse)});
        for (square from : board)
            if (valid_for_pinned[from] & en_passant)
            {
                square ksq = king<side>().front();
                square esq = en_passant.front();
                square psq = side == WHITE ? esq - 8 : esq + 8;
                if (ksq.rank() == psq.rank())
                {
                    bitboard occ = occupied() & ~(bitboard{psq} | bitboard{from});
                    bitboard foo = bitboards::rook_queen(ksq, occ) & rook_queen<~side>();
                    if (foo)
                        continue;
                }
                moves[index++] = {move_t::EN_PASSANT, from, esq};
            }
    };

    generate_king();

    switch (checkers.size())
    {
    case 0:
        [[likely]] generate_castle();
        break;
    case 1:
    {
        square from = king<side>().front();
        square to = checkers.front();
        valids = bitboards::line(from, to);
        if (side == BLACK)
        {
            if (checkers == en_passant << 8)
                validse = en_passant;
        }
        else
        {
            if (checkers == en_passant >> 8)
                validse = en_passant;
        }
    }
    break;
    default:
        return moves.first(index);
    }

    generate_pinned();

    generate_knight();
    generate_rook_queen();
    generate_bishop_queen();
    generate_pawn();

    return moves.first(index);
}

template <side_e side>
void node::execute(const move_t& move) noexcept
{
    move_ = &move;
    dirty_pieces_size = 1;

    const auto remove = [&](bitboard to) noexcept
    {
        for (type_e type : {QUEEN, ROOK, BISHOP, KNIGHT, PAWN})
            if (occupied_by_type[type] != occupied_by_type[type].reset(to))
            {
                piece piece{~side, type};
                dirty_pieces_size = 2;
                dirty_pieces_data[1] = {move.to(), NO_SQUARE, piece};
                hash_ ^= hashes::hash(piece, move.to());
                rule50 = 0;
                break;
            }
        occupied_by_side[~side].reset(to);
        if constexpr (side == WHITE)
        {
            castle.reset(bitboard{"a8h8"_b & to});
        }
        else
        {
            castle.reset(bitboard{"a1h1"_b & to});
        }
    };

    const bitboard from{move.from()};
    const bitboard to{move.to()};
    const bitboard squares{from | to};
    en_passant = 0ull;

    const auto execute = [&](type_e type) noexcept
    {
        occupied_by_type[type].flip(squares);
        occupied_by_side[side].flip(squares);
        piece piece{side, type};
        hash_ ^= hashes::hash(piece, move.from()) ^ hashes::hash(piece, move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece};
    };

    const auto execute_king = [&]() noexcept
    {
        remove(to);
        execute(KING);
        if constexpr (side == WHITE) {
            castle.reset("a1h1"_b);
        } else {
            castle.reset("a8h8"_b);
        }
    };

    const auto execute_castle_short = [&]() noexcept
    {
        if constexpr (side == WHITE)
        {
            occupied_by_type[KING].flip("e1g1"_b);
            occupied_by_type[ROOK].flip("h1f1"_b);
            occupied_by_side[WHITE].flip("e1f1g1h1"_b);
            castle.reset("a1h1"_b);
            hash_ ^= hashes::hash(WKING, E1) ^ hashes::hash(WKING, G1) ^ hashes::hash(WROOK, H1) ^ hashes::hash(WROOK, F1);	
            dirty_pieces_size = 2;
            dirty_pieces_data[0] = {"e1"_s, "g1"_s, WKING};
            dirty_pieces_data[1] = {"h1"_s, "f1"_s, WROOK};
        }
        else
        {
            occupied_by_type[KING].flip("e8g8"_b);
            occupied_by_type[ROOK].flip("h8f8"_b);
            occupied_by_side[BLACK].flip("e8f8g8h8"_b);
            castle.reset("a8h8"_b);
            hash_ ^= hashes::hash(BKING, E8) ^ hashes::hash(BKING, G8) ^ hashes::hash(BROOK, H8) ^ hashes::hash(BROOK, F8);	
            dirty_pieces_size = 2;
            dirty_pieces_data[0] = {"e8"_s, "g8"_s, BKING};
            dirty_pieces_data[1] = {"h8"_s, "f8"_s, BROOK};
        }
    };

    const auto execute_castle_long = [&]() noexcept
    {
        if constexpr (side == WHITE)
        {
            occupied_by_type[KING].flip("e1c1"_b);
            occupied_by_type[ROOK].flip("a1d1"_b);
            occupied_by_side[WHITE].flip("a1c1d1e1"_b);
            castle.reset("a1h1"_b);
            hash_ ^= hashes::hash(WKING, E1) ^ hashes::hash(WKING, C1) ^ hashes::hash(WROOK, A1) ^ hashes::hash(WROOK, D1);
            dirty_pieces_size = 2;
            dirty_pieces_data[0] = {"e1"_s, "c1"_s, WKING};
            dirty_pieces_data[1] = {"a1"_s, "d1"_s, WROOK};
        }
        else
        {
            occupied_by_type[KING].flip("e8c8"_b);
            occupied_by_type[ROOK].flip("a8d8"_b);
            occupied_by_side[BLACK].flip("a8c8d8e8"_b);
            castle.reset("a8h8"_b);
            hash_ ^= hashes::hash(BKING, E8) ^ hashes::hash(BKING, C8) ^ hashes::hash(BROOK, A8) ^ hashes::hash(BROOK, D8);
            dirty_pieces_size = 2;
            dirty_pieces_data[0] = {"e8"_s, "c8"_s, BKING};
            dirty_pieces_data[1] = {"a8"_s, "d8"_s, BROOK};
        }
    };

    const auto execute_queen = [&]() noexcept
    {
        remove(to);
        execute(QUEEN);
    };

    const auto execute_rook = [&]() noexcept
    {
        remove(to);
        execute(ROOK);
        if constexpr (side == WHITE) {
            castle.reset(bitboard{"a1h1"_b & from});
        } else {
            castle.reset(bitboard{"a8h8"_b & from});
        }
    };

    const auto execute_bishop = [&]() noexcept
    {
        remove(to);
        execute(BISHOP);
    };

    const auto execute_knight = [&]() noexcept
    {
        remove(to);
        execute(KNIGHT);
    };

    const auto execute_pawn = [&]() noexcept
    {
        remove(to);
        execute(PAWN);
        rule50 = 0;
    };

    const auto execute_promote = [&](type_e type) noexcept
    {
        remove(to);
        occupied_by_type[PAWN].flip(from);
        occupied_by_type[type].flip(to);
        occupied_by_side[side].flip(squares);
        hash_ ^= hashes::hash(piece{side, PAWN}, move.from()) ^ hashes::hash(piece{side, type}, move.to());
        dirty_pieces_size = 3;
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
        dirty_pieces_data[2] = {NO_SQUARE, move.to(), piece{side, type}};
        rule50 = 0;
    };

    const auto execute_double_push = [&]() noexcept
    {
        execute(PAWN);
        if constexpr (side == WHITE) {
            en_passant = to >> 8;
        } else {
            en_passant = to << 8;
        }
        rule50 = 0;
    };

    const auto execute_en_passant = [&]() noexcept
    {
        execute(PAWN);
        if constexpr (side == WHITE) {
            remove(to >> 8);
        } else {
            remove(to << 8);
        }
        rule50 = 0;
    };

    switch (move.type())
    {
    case move_t::KING:
        execute_king();
        break;
    case move_t::CASTLE_SHORT:
        execute_castle_short();
        break;
    case move_t::CASTLE_LONG:
        execute_castle_long();
        break;
    case move_t::QUEEN:
        execute_queen();
        break;
    case move_t::ROOK:
        execute_rook();
        break;
    case move_t::BISHOP:
        execute_bishop();
        break;
    case move_t::KNIGHT:
        execute_knight();
        break;
    case move_t::PAWN:
        execute_pawn();
        break;
    case move_t::PROMOTE_QUEEN:
        execute_promote(QUEEN);
        break;
    case move_t::PROMOTE_ROOK:
        execute_promote(ROOK);
        break;
    case move_t::PROMOTE_BISHOP:
        execute_promote(BISHOP);
        break;
    case move_t::PROMOTE_KNIGHT:
        execute_promote(KNIGHT);
        break;
    case move_t::DOUBLE_PUSH:
        execute_double_push();
        break;
    case move_t::EN_PASSANT:
        execute_en_passant();
        break;
    }
}

template <side_e side>
void node::execute(std::string_view move_) {
    static const std::regex regex("([a-h][1-8])([a-h][1-8])([qrbn]?)");
    std::cmatch match;
    if (!std::regex_search(&*move_.begin(), &*move_.end(), match, regex))
        throw std::runtime_error("move not matched by regex");
    square from{match[1].str()};
    square to{match[2].str()};
    move_t move{};
    if (match[3].matched && match[3].length() > 0) {
        switch (match[3].str()[0]) {
        case 'q':
            move = {move_t::PROMOTE_QUEEN, from, to};
            break;
        case 'r':
            move = {move_t::PROMOTE_ROOK, from, to};
            break;
        case 'b':
            move = {move_t::PROMOTE_BISHOP, from, to};
            break;
        case 'n':
            move = {move_t::PROMOTE_KNIGHT, from, to};
            break;
        default:
            break;
        }
    } else if (occupied_by_type[KING] & bitboard{from}) {
        if (from.file() - to.file() == 2)
            move = {move_t::CASTLE_LONG, from, to};
        else if (to.file() - from.file() == 2)
            move = {move_t::CASTLE_SHORT, from, to};
        else
            move = {move_t::KING, from, to};
    } else if (occupied_by_type[QUEEN] & occupied_by_side[side] & bitboard{from}) {
        move = {move_t::QUEEN, from, to};
    } else if (occupied_by_type[ROOK] & occupied_by_side[side] & bitboard{from}) {
        move = {move_t::ROOK, from, to};
    } else if (occupied_by_type[BISHOP] & occupied_by_side[side] & bitboard{from}) {
        move = {move_t::BISHOP, from, to};
    } else if (occupied_by_type[KNIGHT] & occupied_by_side[side] & bitboard{from}) {
        move = {move_t::KNIGHT, from, to};
    } else if (occupied_by_type[PAWN] & occupied_by_side[side] & bitboard{from}) {
        // if (occupied_by_type[PAWN] & occupied_by_side[~side] & bitboard{to} == en_passant) {
        if (bitboard{to} == en_passant) {
            move = {move_t::EN_PASSANT, from, to};
        } else if (std::abs(from.rank() - to.rank()) == 2) {
            move = {move_t::DOUBLE_PUSH, from, to};
        } else {
            move = {move_t::PAWN, from, to};
        }
    } else {
        throw std::runtime_error("move not matched by regex");
    }

    execute<side>(move);
}

node::node(std::string_view fen, side_e& side)
: occupied_by_side{0ull, 0ull}, occupied_by_type{0ull, 0ull, 0ull, 0ull, 0ull}, castle{0ull}, en_passant{0ull}, hash_{0ull}, parent_{nullptr}, move_{nullptr}, dirty_pieces_size{0}, rule50{0}
 {
   static const std::regex fen_regex("(.*)/(.*)/(.*)/(.*)/(.*)/(.*)/(.*)/(.*) ([wb]) ([-KQkq]+) ([-a-h1-8]+)( \\d+)?( \\d+)?");

  std::cmatch match;
  if (!std::regex_search(&*fen.begin(), &*fen.end(), match, fen_regex))
    throw std::runtime_error("fen not matched by regex");

  for (int rank = 0; rank < 8; ++rank) {
    int file = 0;
    for (auto ch : match[8 - rank].str()) {
      if (std::isdigit(ch))
        file += ch - '0';
      else {
        square square{static_cast<file_e>(file), static_cast<rank_e>(rank)};
        bitboard board{square};
        switch (ch) {
        case 'K':
          occupied_by_type[KING] |= board;
          occupied_by_side[WHITE] |= board;
          hash_ ^= hashes::hash(WKING, square);
          break;
        case 'k':
          occupied_by_type[KING] |= board;
          occupied_by_side[BLACK] |= board;
          hash_ ^= hashes::hash(BKING, square);
          break;
        case 'Q':
          occupied_by_type[QUEEN] |= board;
          occupied_by_side[WHITE] |= board;
          hash_ ^= hashes::hash(WQUEEN, square);
          break;
        case 'q':
          occupied_by_type[QUEEN] |= board;
          occupied_by_side[BLACK] |= board;
          hash_ ^= hashes::hash(BQUEEN, square);
          break;
        case 'R':
          occupied_by_type[ROOK] |= board;
          occupied_by_side[WHITE] |= board;
          hash_ ^= hashes::hash(WROOK, square);
          break;
        case 'r':
          occupied_by_type[ROOK] |= board;
          occupied_by_side[BLACK] |= board;
          hash_ ^= hashes::hash(BROOK, square);
          break;
        case 'B':
          occupied_by_type[BISHOP] |= board;
          occupied_by_side[WHITE] |= board;
          hash_ ^= hashes::hash(WBISHOP, square);
          break;
        case 'b':
          occupied_by_type[BISHOP] |= board;
          occupied_by_side[BLACK] |= board;
          hash_ ^= hashes::hash(BBISHOP, square);
          break;
        case 'N':
          occupied_by_type[KNIGHT] |= board;
          occupied_by_side[WHITE] |= board;
          hash_ ^= hashes::hash(WKNIGHT, square);
          break;
        case 'n':
          occupied_by_type[KNIGHT] |= board;
          occupied_by_side[BLACK] |= board;
          hash_ ^= hashes::hash(BKNIGHT, square);
          break;
        case 'P':
          occupied_by_type[PAWN] |= board;
          occupied_by_side[WHITE] |= board;
          hash_ ^= hashes::hash(WPAWN, square);
          break;
        case 'p':
          occupied_by_type[PAWN] |= board;
          occupied_by_side[BLACK] |= board;
          hash_ ^= hashes::hash(BPAWN, square);
          break;
        }
        ++file;
      }
    }
  }
  side = match[9].str()[0] == 'w' ? WHITE : BLACK;
  if (match[10].compare("-")) {
    for (auto ch : match[10].str()) {
      switch (ch) {
      case 'K':
        castle |= "h1"_b;
        break;
      case 'Q':
        castle |= "a1"_b;
        break;
      case 'k':
        castle |= "h8"_b;
        break;
      case 'q':
        castle |= "a8"_b;
        break;
      }
    }
  }
  if (match[11].compare("-")) {
    en_passant = bitboard{match[11].str()};
  }
  // todo: rule50
}
