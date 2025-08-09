#pragma once

#include "nnue/nnue.hpp"
#include "position.hpp"
#include <print>

class evaluator {
    using NNUE = nnue::big_nnue;
    using Entry = NNUE::Entry;

    NNUE nnue;
    mutable Entry entries[SIDE_MAX][SQUARE_MAX];

    const Entry& refresh(const position_t& position, side_e side) const noexcept {
        auto king = position.by(side, KING).front();
        auto& entry = entries[side][king];
        std::uint16_t added_features[32];
        std::uint16_t removed_features[32];
        std::size_t added_count = 0;
        std::size_t removed_count = 0;
        for (piece piece : {WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING}) {
            bitboard oldBB = entry.pieces[piece.type()] & entry.colors[piece.side()];
            bitboard newBB = position.by(piece);
            for (square square : oldBB & ~newBB) {
                removed_features[removed_count++] = nnue::make_index(king, square, piece, side);
            }
            for (square square : newBB & ~oldBB) {
                added_features[added_count++] = nnue::make_index(king, square, piece, side);
            }
        }

        nnue.update(entry, std::span{removed_features}.first(removed_count), std::span{added_features}.first(added_count));

        for (side_e c : {WHITE, BLACK})
            entry.colors[c] = position.by(c);

        for (type_e p : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING})
            entry.pieces[p] = position.by(p);

        return entry;
    }

public:
    evaluator() : nnue{} {
        for (side_e side : {WHITE, BLACK}) {
            for (Entry& entry : entries[side]) {
                nnue.initialize(entry);
            }
        }
    }

private:
    // Sigmoid function: maps (-∞, +∞) to (-1, +1)
    static float sigmoid(float x, float scale = 1.0f) {
        return std::tanh(x / scale);
    }
    
    // Alternative: maps (-∞, +∞) to (0, 1)
    static float sigmoid_01(float x, float scale = 1.0f) {
        return 1.0f / (1.0f + std::exp(-x / scale));
    }
    
    // Maps sigmoid output to centipawn range
    static int sigmoid_to_cp(float sigmoid_value, int max_cp = 2000) {
        return static_cast<int>(sigmoid_value * max_cp);
    }

public:
    std::int32_t evaluate(const position_t& position) const noexcept {
        const Entry& t = refresh(position, position.get_side());
        const Entry& o = refresh(position, ~position.get_side());
        int raw_nnue = nnue.evaluate(t, o, position.by().size());// & ~15;
        float sigmoid_value = sigmoid(raw_nnue, 8000.f);
        return sigmoid_to_cp(sigmoid_value, 500);
        // return raw_nnue;
    }


};
