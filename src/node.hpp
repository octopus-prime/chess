#pragma once

#include "square.hpp"
#include "piece.hpp"
#include "side.hpp"
#include "bitboard.hpp"
#include "bitboards.hpp"
// #include "hash.hpp"
// #include "hashes.hpp"
#include "move.hpp"
// #include "nnue.hpp"
#include "dirty_piece.hpp"

class node
{
    bitboard occupied_by_side[SIDE_MAX];
    bitboard occupied_by_type[TYPE_MAX];
    // bitboard white;
    // bitboard black;
    // bitboard king_;
    // bitboard rook_queen_;
    // bitboard bishop_queen_;
    // bitboard knight_;
    // bitboard pawn_;
    bitboard castle;
    bitboard en_passant;
    // hash_t hash_;
    const node* parent_;
    const move_t* move_;
    size_t dirty_pieces_size;
    dirty_piece dirty_pieces_data[3];

    auto get_dirty_pieces() const noexcept {
        return std::span{dirty_pieces_data}.first(dirty_pieces_size);
    }

public:
    // NNUEdata nnue;

    enum generation_t {all, captures};

    constexpr node() noexcept
    : 
    occupied_by_side{"12"_r, "78"_r},
    occupied_by_type{0, "27"_r, "b1g1b8g8"_b, "c1f1c8f8"_b, "a1h1a8h8"_b, "d1d8"_b, "e1e8"_b, 0},
    castle{"a1h1a8h8"_b}, en_passant{0}, parent_{nullptr}, move_{nullptr}, dirty_pieces_size{0}
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
        : castle{parent.castle}, en_passant{parent.en_passant}, parent_(&parent), move_(nullptr), dirty_pieces_size{0}
        {
            std::ranges::copy(parent.occupied_by_side, this->occupied_by_side);
            std::ranges::copy(parent.occupied_by_type, this->occupied_by_type);
        }

    // constexpr node(bitboard white, bitboard black, bitboard king, bitboard rook_queen, bitboard bishop_queen, bitboard knight, bitboard pawn, bitboard castle, bitboard en_passant, hash_t hash) noexcept
    //     : white(white), black(black), king_(king), rook_queen_(rook_queen), bishop_queen_(bishop_queen), knight_(knight), pawn_(pawn), castle(castle), en_passant(en_passant), hash_(hash), parent_(nullptr), move_(nullptr) {
    //         nnue.accumulator.computedAccumulation = 0;
    //         nnue.dirtyPiece.dirtyNum = 0;
    //     }

    // constexpr node(node& parent) noexcept
    //     : white(parent.white), black(parent.black), king_(parent.king_), rook_queen_(parent.rook_queen_), bishop_queen_(parent.bishop_queen_), knight_(parent.knight_), pawn_(parent.pawn_), castle(parent.castle), en_passant(0), hash_(parent.hash_), parent_(&parent), move_(nullptr) {
    //         nnue.accumulator.computedAccumulation = 0;
    //         nnue.dirtyPiece.dirtyNum = 0;
    //     }

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

    // template <side_e side>
    // constexpr hash_t hash() const noexcept {
    //     return side == WHITE ? hash_ ^ castle ^ en_passant :  ~(hash_ ^ castle ^ en_passant);
    // }

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

    // template <side_t side>
    std::pair<bitboard, const move_t*> execute(bitboard en_passant, const move_t* move) noexcept {
        auto e = this->en_passant;
        auto m = this->move_;
        this->en_passant = en_passant;
        this->move_ = move;
        // nnue.accumulator.computedAccumulation = 0;
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
                dirty_pieces_size = 2;
                dirty_pieces_data[1] = {move.to(), static_cast<square_e>(64), piece{~side, type}};
                break;
            }
        // if (occupied_by_type[type_e::queen] != occupied_by_type[type_e::queen].reset(to)) {
        //     // hash_ ^= hashes::knight<~side>(to.find());
        //     dirty_pieces_size = 2;
        //     dirty_pieces_data[1].from = move.to();
        //     dirty_pieces_data[1].to = static_cast<square_e>(64);
        //     dirty_pieces_data[1].piece = piece{~side, type_e::queen};
        // }

        // bool r = rook_queen_ != rook_queen_.reset(to);
        // bool b = bishop_queen_ != bishop_queen_.reset(to);
        // switch(r + b * 2) {
        // case 1:
        //     hash_ ^= hashes::rook<~side>(to.find());
        //     nnue.dirtyPiece.dirtyNum = 2;
        //     nnue.dirtyPiece.pc[1] = side == WHITE ? brook : wrook;
        //     nnue.dirtyPiece.from[1] = move.to();
        //     nnue.dirtyPiece.to[1] = 64;
        //     break;
        // case 2:
        //     hash_ ^= hashes::bishop<~side>(to.find());
        //     nnue.dirtyPiece.dirtyNum = 2;
        //     nnue.dirtyPiece.pc[1] = side == WHITE ? bbishop : wbishop;
        //     nnue.dirtyPiece.from[1] = move.to();
        //     nnue.dirtyPiece.to[1] = 64;
        //     break;
        // case 3:
        //     hash_ ^= hashes::queen<~side>(to.find());
        //     nnue.dirtyPiece.dirtyNum = 2;
        //     nnue.dirtyPiece.pc[1] = side == WHITE ? bqueen : wqueen;
        //     nnue.dirtyPiece.from[1] = move.to();
        //     nnue.dirtyPiece.to[1] = 64;
        //     break;
        // default: 
        //     break;
        // }
        // if (knight_ != knight_.reset(to)) {
        //     hash_ ^= hashes::knight<~side>(to.find());
        //     nnue.dirtyPiece.dirtyNum = 2;
        //     nnue.dirtyPiece.pc[1] = side == WHITE ? bknight : wknight;
        //     nnue.dirtyPiece.from[1] = move.to();
        //     nnue.dirtyPiece.to[1] = 64;
        // }
        // if (pawn_ != pawn_.reset(to)) {
        //     hash_ ^= hashes::pawn<~side>(to.find());
        //     nnue.dirtyPiece.dirtyNum = 2;
        //     nnue.dirtyPiece.pc[1] = side == WHITE ? bpawn : wpawn;
        //     nnue.dirtyPiece.from[1] = move.to();
        //     nnue.dirtyPiece.to[1] = 64;
        // }
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

    const auto execute_king = [&]() noexcept
    {
        remove(to);
        occupied_by_type[KING].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::king<side>(move.from()) ^ hashes::king<side>(move.to());
        if constexpr (side == WHITE)
        {
            castle.reset("a1h1"_b);
            dirty_pieces_data[0] = {move.from(), move.to(), WKING};
        }
        else
        {
            castle.reset("a8h8"_b);
            dirty_pieces_data[0] = {move.from(), move.to(), BKING};
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
            // hash_ ^= hashes::king<side>("e1"_s) ^ hashes::king<side>("g1"_s) ^ hashes::rook<side>("h1"_s) ^ hashes::rook<side>("f1"_s);
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
            // hash_ ^= hashes::king<side>("e8"_s) ^ hashes::king<side>("g8"_s) ^ hashes::rook<side>("h8"_s) ^ hashes::rook<side>("f8"_s);
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
            // hash_ ^= hashes::king<side>("e1"_s) ^ hashes::king<side>("c1"_s) ^ hashes::rook<side>("a1"_s) ^ hashes::rook<side>("d1"_s);
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
            // hash_ ^= hashes::king<side>("e8"_s) ^ hashes::king<side>("c8"_s) ^ hashes::rook<side>("a8"_s) ^ hashes::rook<side>("d8"_s);
            dirty_pieces_size = 2;
            dirty_pieces_data[0] = {"e8"_s, "c8"_s, BKING};
            dirty_pieces_data[1] = {"a8"_s, "d8"_s, BROOK};
        }
    };

    const auto execute_knight = [&]() noexcept
    {
        remove(to);
        occupied_by_type[KNIGHT].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::knight<side>(move.from()) ^ hashes::knight<side>(move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, KNIGHT}};
    };

    const auto execute_queen = [&]() noexcept
    {
        remove(to);
        occupied_by_type[QUEEN].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::queen<side>(move.from()) ^ hashes::queen<side>(move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, QUEEN}};
    };

    const auto execute_rook = [&]() noexcept
    {
        remove(to);
        occupied_by_type[ROOK].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::rook<side>(move.from()) ^ hashes::rook<side>(move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, ROOK}};
        if constexpr (side == WHITE)
        {
            castle.reset(bitboard{"a1h1"_b & from});
        }
        else
        {
            castle.reset(bitboard{"a8h8"_b & from});
        }
    };

    const auto execute_bishop = [&]() noexcept
    {
        remove(to);
        occupied_by_type[BISHOP].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::bishop<side>(move.from()) ^ hashes::bishop<side>(move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, BISHOP}};
    };

    const auto execute_pawn = [&]() noexcept
    {
        remove(to);
        occupied_by_type[PAWN].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::pawn<side>(move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
    };

    // const auto execute_promote_queen = [&]() noexcept
    // {
    //     remove(to);
    //     occupied_by_type[PAWN].flip(from);
    //     occupied_by_type[QUEEN].flip(to);
    //     occupied_by_side[side].flip(squares);
    //     // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::queen<side>(move.to());
    //     dirty_pieces_size = 3;
    //     dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
    //     dirty_pieces_data[2] = {static_cast<square_e>(64), move.to(), piece{side, QUEEN}};
    // };

    // const auto execute_promote_rook = [&]() noexcept
    // {
    //     remove(to);
    //     occupied_by_type[PAWN].flip(from);
    //     occupied_by_type[ROOK].flip(to);
    //     occupied_by_side[side].flip(squares);
    //     // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::rook<side>(move.to());
    //     dirty_pieces_size = 3;
    //     dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
    //     dirty_pieces_data[2] = {static_cast<square_e>(64), move.to(), piece{side, ROOK}};
    // };

    // const auto execute_promote_bishop = [&]() noexcept
    // {
    //     remove(to);
    //     occupied_by_type[PAWN].flip(from);
    //     occupied_by_type[BISHOP].flip(to);
    //     occupied_by_side[side].flip(squares);
    //     // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::bishop<side>(move.to());
    //     dirty_pieces_size = 3;
    //     dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
    //     dirty_pieces_data[2] = {static_cast<square_e>(64), move.to(), piece{side, BISHOP}};
    // };

    // const auto execute_promote_knight = [&]() noexcept
    // {
    //     remove(to);
    //     occupied_by_type[PAWN].flip(from);
    //     occupied_by_type[KNIGHT].flip(to);
    //     occupied_by_side[side].flip(squares);
    //     // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::knight<side>(move.to());
    //     dirty_pieces_size = 3;
    //     dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
    //     dirty_pieces_data[2] = {static_cast<square_e>(64), move.to(), piece{side, KNIGHT}};
    // };

    const auto execute_promote = [&](type_e type) noexcept
    {
        remove(to);
        occupied_by_type[PAWN].flip(from);
        occupied_by_type[type].flip(to);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::knight<side>(move.to());
        dirty_pieces_size = 3;
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
        dirty_pieces_data[2] = {static_cast<square_e>(64), move.to(), piece{side, type}};
    };

    const auto execute_double_push = [&]() noexcept
    {
        occupied_by_type[PAWN].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::pawn<side>(move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
        if constexpr (side == WHITE)
        {
            en_passant = to >> 8;
        }
        else
        {
            en_passant = to << 8;
        }
    };

    const auto execute_en_passant = [&]() noexcept
    {
        occupied_by_type[PAWN].flip(squares);
        occupied_by_side[side].flip(squares);
        // hash_ ^= hashes::pawn<side>(move.from()) ^ hashes::pawn<side>(move.to());
        dirty_pieces_data[0] = {move.from(), move.to(), piece{side, PAWN}};
        if constexpr (side == WHITE)
        {
            remove(to >> 8);
        }
        else
        {
            remove(to << 8);
        }
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
    case move_t::KNIGHT:
        execute_knight();
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

node::node(std::string_view fen, side_e& side)
: occupied_by_side{0ull, 0ull}, occupied_by_type{0ull, 0ull, 0ull, 0ull, 0ull}, castle{0ull}, en_passant{0ull}
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
        //   hash ^= hashes::king<WHITE>(square);
          break;
        case 'k':
          occupied_by_type[KING] |= board;
          occupied_by_side[BLACK] |= board;
        //   hash ^= hashes::king<BLACK>(square);
          break;
        case 'Q':
          occupied_by_type[QUEEN] |= board;
          occupied_by_side[WHITE] |= board;
        //   hash ^= hashes::queen<WHITE>(square);
          break;
        case 'q':
          occupied_by_type[QUEEN] |= board;
          occupied_by_side[BLACK] |= board;
        //   hash ^= hashes::queen<BLACK>(square);
          break;
        case 'R':
          occupied_by_type[ROOK] |= board;
          occupied_by_side[WHITE] |= board;
        //   hash ^= hashes::rook<WHITE>(square);
          break;
        case 'r':
          occupied_by_type[ROOK] |= board;
          occupied_by_side[BLACK] |= board;
        //   hash ^= hashes::rook<BLACK>(square);
          break;
        case 'B':
          occupied_by_type[BISHOP] |= board;
          occupied_by_side[WHITE] |= board;
        //   hash ^= hashes::bishop<WHITE>(square);
          break;
        case 'b':
          occupied_by_type[BISHOP] |= board;
          occupied_by_side[BLACK] |= board;
        //   hash ^= hashes::bishop<BLACK>(square);
          break;
        case 'N':
          occupied_by_type[KNIGHT] |= board;
          occupied_by_side[WHITE] |= board;
        //   hash ^= hashes::knight<WHITE>(square);
          break;
        case 'n':
          occupied_by_type[KNIGHT] |= board;
          occupied_by_side[BLACK] |= board;
        //   hash ^= hashes::knight<BLACK>(square);
          break;
        case 'P':
          occupied_by_type[PAWN] |= board;
          occupied_by_side[WHITE] |= board;
        //   hash ^= hashes::pawn<WHITE>(square);
          break;
        case 'p':
          occupied_by_type[PAWN] |= board;
          occupied_by_side[BLACK] |= board;
        //   hash ^= hashes::pawn<BLACK>(square);
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
}
