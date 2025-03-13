#pragma once

#include "nnue/nnue.hpp"
#include "node.hpp"
#include <print>

class evaluator {
    using NNUE = nnue::big_nnue;
    using Accumulator = NNUE::Accumulator;
    using Entry = NNUE::Entry;

    NNUE nnue;
    mutable Entry entries[64][2];

    template <side_e Perspective, bool Cache>
    void refresh(const node& position) const noexcept {
        auto king = position.king<Perspective>().front();
        auto& accumulator = position.get_accumulator();

    if constexpr (!Cache) {
        std::uint16_t active_features_data[32];
        std::size_t active_features_size = 0;

        for (piece piece : enum_range(WPAWN, BKING))
            for (square square: position.board(piece))
                active_features_data[active_features_size++] = nnue::make_index<Perspective>(king, square, piece);

        nnue.refresh<Perspective>(accumulator, std::span{active_features_data}.first(active_features_size));
    } else {
        // auto& entry = entries[king][Perspective];
        // std::uint16_t added_features[32];
        // std::uint16_t removed_features[32];
        // std::size_t added_count = 0;
        // std::size_t removed_count = 0;
        // for (auto c : {WHITE, BLACK}) {
        //     for (auto p : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
        //         auto piece = 8 * c + p;
        //         auto oldBB = entry.colors[c] & entry.pieces[p];
        //         auto newBB = position.get_colors(c) & position.get_pieces(p);
        //         for (auto bb = oldBB & ~newBB; bb; bb &= bb - 1)
        //             removed_features[removed_count++] = make_index<Perspective>(king, std::countr_zero(bb), piece);

        //         for (auto bb = newBB & ~oldBB; bb; bb &= bb - 1)
        //             added_features[added_count++] = make_index<Perspective>(king, std::countr_zero(bb), piece);
        //     }
        // }

        // nnue.update(entry, std::span{removed_features}.first(removed_count), std::span{added_features}.first(added_count));

        // for (auto c : {WHITE, BLACK})
        //     entry.colors[c] = position.get_colors(c);

        // for (auto p : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING})
        //     entry.pieces[p] = position.get_pieces(p);

        // std::ranges::copy(span_cast<const __m256i>(std::span{entry.accumulation}), span_cast<__m256i>(std::span{accumulator.accumulation[Perspective]}).begin());
    }
    }

    template <side_e Perspective>
    void update(const node& position) const noexcept {
        const auto king = position.king<Perspective>().front();
        auto& accumulator = position.get_accumulator();
        const auto& previous = position.parent()->get_accumulator();

        std::uint16_t removed_features_data[3];
        std::uint16_t added_features_data[3];
        std::size_t removed_features_size = 0;
        std::size_t added_features_size = 0;

        for (const auto& dirty_piece : position.get_dirty_pieces()) {
            if (dirty_piece.from != NO_SQUARE)
                removed_features_data[removed_features_size++] = nnue::make_index<Perspective>(king, dirty_piece.from, dirty_piece.piece);
            if (dirty_piece.to != NO_SQUARE)
                added_features_data[added_features_size++] = nnue::make_index<Perspective>(king, dirty_piece.to, dirty_piece.piece);
        }

        nnue.update<Perspective>(accumulator, previous, std::span{removed_features_data}.first(removed_features_size), std::span{added_features_data}.first(added_features_size));
    }

    template <side_e Perspective, bool Recursive>
    void prepare(const node& position) const noexcept {
        auto& accumulator = position.get_accumulator();

        if (accumulator.computed[Perspective])
            return;

        const node* parent = position.parent();
        if (!Recursive || parent == nullptr || position.get_dirty_pieces()[0].piece.type() == KING) {
            refresh<Perspective, false>(position);
        } else {
            prepare<Perspective, true>(*parent);
            update<Perspective>(position);
        }

        accumulator.computed[Perspective] = true;
    }

public:
    evaluator() : nnue{} {
        for (auto& entry : entries) {
            for (auto c : {WHITE, BLACK})
                nnue.initialize(entry[c]);
        }
    }

    std::int32_t evaluate(const node& position, const move_t& move) const noexcept {
        // auto removed = std::ranges::find_if({QUEEN, ROOK, BISHOP, KNIGHT, PAWN}, [&](type_e type) {
        constexpr std::array values {0, 100, 300, 300, 500, 900};
        constexpr std::array types {PAWN, KNIGHT, BISHOP, ROOK, QUEEN};
        auto removed = std::ranges::find_if(types, [&](type_e type) {
            return position.board(type)[move.to()];
        });
        auto foo = removed == types.end() ? 0 : values[*removed];
        switch (move.type())
        {
        case move_t::KING:
            return -5 + foo;
        case move_t::CASTLE_SHORT:
            return 0;
        case move_t::CASTLE_LONG:
            return 0;
        case move_t::KNIGHT:
            return -1 + foo;
        case move_t::QUEEN:
            return -4 + foo;
        case move_t::ROOK:
            return -3 + foo;
        case move_t::BISHOP:
            return -2 + foo;
        case move_t::PAWN:
        case move_t::DOUBLE_PUSH:
            return 0 + foo;
        case move_t::PROMOTE_QUEEN:
            return 800 + foo;
        case move_t::PROMOTE_ROOK:
            return 400  + foo;
        case move_t::PROMOTE_BISHOP:
            return 200 + foo;
        case move_t::PROMOTE_KNIGHT:
            return 200  + foo;
        case move_t::EN_PASSANT:
            return 100;
        }
    }

    template <side_e Perspective>
    std::int32_t evaluate(const node& position) const noexcept {
        prepare<WHITE, true>(position);
        prepare<BLACK, true>(position);
        auto v = nnue.evaluate<Perspective>(position.get_accumulator(), position.occupied().size());// & ~15;
        // v -= v * position.get_rule50() / 212;
        return v;
    }
};
