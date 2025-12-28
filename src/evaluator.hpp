#pragma once

#include "nnue/nnue.hpp"
#include "position.hpp"
#include "score.hpp"

class evaluator {
    template <typename NNUE>
    class nnue_evaluator {
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
        nnue_evaluator() : nnue{} {
            for (side_e side : {WHITE, BLACK}) {
                for (Entry& entry : entries[side]) {
                    nnue.initialize(entry);
                }
            }
        }

        score_t evaluate(const position_t& position) const noexcept {
            const Entry& t = refresh(position, position.get_side());
            const Entry& o = refresh(position, ~position.get_side());
            return nnue.evaluate(t, o, position.by().size());
        }
    };

    nnue_evaluator<nnue::small_nnue> small;
    nnue_evaluator<nnue::big_nnue> big;

public:
    evaluator() : small{}, big{} {
    }

    score_t evaluate(const position_t& position, score_t alpha, score_t beta) const noexcept {
        constexpr score_t threshold = 150;
        score_t score = small.evaluate(position);
        score_t lower = alpha - threshold;
        score_t upper = beta + threshold;
        if (lower < score && score < upper) {
            score = big.evaluate(position);
        }
        score -= score * position.get_half_move() / 212;     // Damp down the evaluation linearly when shuffling
        return score;
    }
};
