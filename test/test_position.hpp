#pragma once

#include <position.hpp>
#include "ut.hpp"

void test_position() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    // note: perft tests

    "check"_test = [] {
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - -"}.is_check(), false));
        ut::expect(ut::eq(position_t{"2Q3n1/k2R4/p7/5p2/1pN5/5P2/PPP5/2K1R3 b - -"}.is_check(), true));
    };

    "move_check"_test = [] {
        position_t position{"5rk1/p2b4/P7/3n4/8/2p5/1P2P1q1/3K4 b - -"};
        ut::expect(ut::eq(position.check("g2e2"_m), true));
        ut::expect(ut::eq(position.check("g2f1"_m), true));
        ut::expect(ut::eq(position.check("g2g1"_m), true));
        ut::expect(ut::eq(position.check("g2h1"_m), true));
        ut::expect(ut::eq(position.check("g2f3"_m), false));
        ut::expect(ut::eq(position.check("f8f1"_m), true));
        ut::expect(ut::eq(position.check("f8d8"_m), false));
        ut::expect(ut::eq(position.check("d7a4"_m), true));
        ut::expect(ut::eq(position.check("d7g4"_m), false));
        ut::expect(ut::eq(position.check("d5e3"_m), true));
        ut::expect(ut::eq(position.check("d5b4"_m), false));
        ut::expect(ut::eq(position.check("c3c2"_m), true));
        ut::expect(ut::eq(position.check("c3b2"_m), false));
        position = "3k4/1p2p1Q1/2P5/8/3N4/p7/P2B4/5RK1 w - -"sv;
        ut::expect(ut::eq(position.check("g7e7"_m), true));
        ut::expect(ut::eq(position.check("g7f8"_m), true));
        ut::expect(ut::eq(position.check("g7g8"_m), true));
        ut::expect(ut::eq(position.check("g7h8"_m), true));
        ut::expect(ut::eq(position.check("g7f6"_m), false));
        ut::expect(ut::eq(position.check("f1f8"_m), true));
        ut::expect(ut::eq(position.check("f1d1"_m), false));
        ut::expect(ut::eq(position.check("d2a5"_m), true));
        ut::expect(ut::eq(position.check("d2g5"_m), false));
        ut::expect(ut::eq(position.check("d4e6"_m), true));
        ut::expect(ut::eq(position.check("d4b5"_m), false));
        ut::expect(ut::eq(position.check("c6c7"_m), true));
        ut::expect(ut::eq(position.check("c6b7"_m), false));
    };

    "50_moves_rule"_test = [] {
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - - 99 123"}.is_50_moves_rule(), false));
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - - 100 123"}.is_50_moves_rule(), true));
    };

    "3_fold_repetition"_test = [] {
        position_t position{};
        position.make_move("g1f3"_m);
        position.make_move("g8f6"_m);
        position.make_move("f3g1"_m);
        position.make_move("f6g8"_m);
        position.make_move("g1f3"_m);
        position.make_move("g8f6"_m);
        position.make_move("f3g1"_m);
        ut::expect(ut::eq(position.is_3_fold_repetition(), false)); 
        position.make_move("f6g8"_m);
        ut::expect(ut::eq(position.is_3_fold_repetition(), true));
    };

    "no_material"_test = [] {
        ut::expect(ut::eq(position_t{}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4k3/8/8/8/8/8/8/4K3 w - -"}.is_no_material(), true));

        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4K3 w - -"}.is_no_material(), true));
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - -"}.is_no_material(), true));
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KB2 w - -"}.is_no_material(), true));
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KR2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KNB w - -"}.is_no_material(), false));

        ut::expect(ut::eq(position_t{"4kn2/8/8/8/8/8/8/4K3 w - -"}.is_no_material(), true));
        ut::expect(ut::eq(position_t{"4kn2/8/8/8/8/8/8/4KN2 w - -"}.is_no_material(), true));
        ut::expect(ut::eq(position_t{"4kn2/8/8/8/8/8/8/4KB2 w - -"}.is_no_material(), true));
        ut::expect(ut::eq(position_t{"4kn2/8/8/8/8/8/8/4KR2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4kn2/8/8/8/8/8/8/4KNB w - -"}.is_no_material(), false));

        ut::expect(ut::eq(position_t{"4kr2/8/8/8/8/8/8/4KN2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4kr2/8/8/8/8/8/8/4KB2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4kr2/8/8/8/8/8/8/4KR2 w - -"}.is_no_material(), false));

        ut::expect(ut::eq(position_t{"4kq2/8/8/8/8/8/8/4KN2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4kq2/8/8/8/8/8/8/4KB2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4kq2/8/8/8/8/8/8/4KR2 w - -"}.is_no_material(), false));

        ut::expect(ut::eq(position_t{"4k3/p7/8/8/8/8/8/4KN2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4k3/p7/8/8/8/8/8/4KB2 w - -"}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4k3/p7/8/8/8/8/8/4KR2 w - -"}.is_no_material(), false));
    };

    "see"_test = []() {
        std::ifstream stream{"../epd/see.txt"};
        std::array<char, 256> epd;
        position_t position;
        while (stream.good()) {
            stream.getline(epd.data(), epd.size());
            std::string_view epd_view{epd.data()};

            if (epd_view.empty() || epd_view.starts_with("#")) {
                return;
            }

            auto parts = epd_view | std::views::split("; "sv);
            auto part = parts.begin();

            std::string_view fen_part{*part++};
            position = fen_part;

            std::string_view move_part{*part++};
            move_t move{move_part};

            std::string_view see_part{*part++};
            int see_value;
            std::from_chars(&*see_part.begin(), &*see_part.end(), see_value);

            int see_result = position.see(move);
            ut::expect(ut::eq(see_result, see_value)) << epd_view;
            // std::println("{}: {} -> {}", epd_view, move, see_value);
        }
    };

    "generate_active_moves"_test = [] {
        position_t position{"4kb2/8/8/8/8/8/8/4KN2 w - -"sv};
        std::array<move_t, position_t::MAX_ACTIVE_MOVES_PER_PLY> buffer;
        auto moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::views::empty<move_t>));

        position = "4k3/7P/1N6/3p4/8/8/8/4K3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"b6d5"_m, "h7h8q"_m, "h7h8n"_m}));

        position = "4k1n1/7P/1N6/3p4/8/8/8/4K3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"b6d5"_m, "h7h8q"_m, "h7h8n"_m, "h7g8q"_m, "h7g8n"_m}));

        position = "4k1n1/7P/1N6/3p3N/8/8/8/R2BK3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"b6d5"_m, "h7h8q"_m, "h7h8n"_m, "h7g8q"_m, "h7g8n"_m, "a1a8"_m, "d1a4"_m, "h5g7"_m, "h5f6"_m}));

        position = "4k1nQ/7P/1N1P4/3p3N/8/8/8/R2BK3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"h5f6"_m, "h5g7"_m, "b6d5"_m, "a1a8"_m, "h8g8"_m, "d1a4"_m, "h8e5"_m, "d6d7"_m, "h7g8q"_m, "h7g8n"_m}));

        position = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"g2h3"_m, "f3h3"_m, "f3f6"_m, "e5g6"_m, "e5f7"_m, "e5d7"_m, "d5e6"_m, "e2a6"_m}));

        // position = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -"sv;
        // moves = position.generate_active_moves(buffer);
        // ut::expect(ut::eq(moves.size(), 5));
        // ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"g2h3"_m, "f3h3"_m, "f3f6"_m, "e5g6"_m, "e5f7"_m}));
    };

    "pawn_hash_non_zero"_test = [] {
        // Starting position has pieces, so all hashes must be non-zero
        position_t pos{};
        ut::expect(ut::neq(pos.pawn_hash(),  hash_t{0}));
        ut::expect(ut::neq(pos.minor_hash(), hash_t{0})); // knights + bishops
        ut::expect(ut::neq(pos.major_hash(), hash_t{0})); // rooks + queens
    };

    "pawn_hash_make_undo"_test = [] {
        // After make+undo any move the hashes must be restored
        position_t pos{};
        auto ph0  = pos.pawn_hash();
        auto mnh0 = pos.minor_hash();
        auto mjh0 = pos.major_hash();

        // pawn move — only pawn_hash changes
        pos.make_move("e2e4"_m);
        ut::expect(ut::neq(pos.pawn_hash(),  ph0));   // pawn moved
        ut::expect(ut::eq(pos.minor_hash(), mnh0));   // unchanged
        ut::expect(ut::eq(pos.major_hash(), mjh0));   // unchanged
        pos.undo_move("e2e4"_m);
        ut::expect(ut::eq(pos.pawn_hash(),  ph0));
        ut::expect(ut::eq(pos.minor_hash(), mnh0));
        ut::expect(ut::eq(pos.major_hash(), mjh0));

        // knight move — only minor_hash changes
        pos.make_move("g1f3"_m);
        ut::expect(ut::eq(pos.pawn_hash(),   ph0));   // unchanged
        ut::expect(ut::neq(pos.minor_hash(), mnh0));  // knight moved
        ut::expect(ut::eq(pos.major_hash(),  mjh0));  // unchanged
        pos.undo_move("g1f3"_m);
        ut::expect(ut::eq(pos.pawn_hash(),  ph0));
        ut::expect(ut::eq(pos.minor_hash(), mnh0));
        ut::expect(ut::eq(pos.major_hash(), mjh0));
    };

    "pawn_hash_castling"_test = [] {
        // Castling moves the rook (major) → major_hash must change; pawn/minor must not
        position_t pos{"r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq -"};
        auto ph0  = pos.pawn_hash();
        auto mnh0 = pos.minor_hash();
        auto mjh0 = pos.major_hash();

        pos.make_move("e1g1"_m); // white O-O
        ut::expect(ut::eq(pos.pawn_hash(),   ph0));
        ut::expect(ut::eq(pos.minor_hash(),  mnh0));
        ut::expect(ut::neq(pos.major_hash(), mjh0));
        pos.undo_move("e1g1"_m);
        ut::expect(ut::eq(pos.pawn_hash(),  ph0));
        ut::expect(ut::eq(pos.minor_hash(), mnh0));
        ut::expect(ut::eq(pos.major_hash(), mjh0));
    };

    "pawn_hash_capture_pawn"_test = [] {
        // Capturing a pawn only changes pawn_hash
        position_t pos{"4k3/8/8/3p4/4P3/8/8/4K3 w - -"};
        auto ph0  = pos.pawn_hash();
        auto mnh0 = pos.minor_hash();
        auto mjh0 = pos.major_hash();

        pos.make_move("e4d5"_m); // white pawn captures black pawn
        ut::expect(ut::neq(pos.pawn_hash(),  ph0));   // both pawns moved/gone
        ut::expect(ut::eq(pos.minor_hash(), mnh0));   // no piece involved
        ut::expect(ut::eq(pos.major_hash(), mjh0));   // no piece involved
        pos.undo_move("e4d5"_m);
        ut::expect(ut::eq(pos.pawn_hash(),  ph0));
        ut::expect(ut::eq(pos.minor_hash(), mnh0));
        ut::expect(ut::eq(pos.major_hash(), mjh0));
    };

    "pawn_hash_en_passant"_test = [] {
        // En-passant capture removes the captured pawn → pawn_hash changes
        position_t pos{"4k3/8/8/3pP3/8/8/8/4K3 w - d6"};
        auto ph0 = pos.pawn_hash();

        pos.make_move("e5d6"_m); // white captures en passant
        ut::expect(ut::neq(pos.pawn_hash(), ph0));
        pos.undo_move("e5d6"_m);
        ut::expect(ut::eq(pos.pawn_hash(), ph0));
    };

    "pawn_hash_promotion"_test = [] {
        // Queen promotion: pawn_hash changes + major_hash changes, minor_hash unchanged
        position_t pos{"4k3/P7/8/8/8/8/8/4K3 w - -"};
        auto ph0 = pos.pawn_hash();
        auto mnh0 = pos.minor_hash(); // 0 — no minor pieces
        auto mjh0 = pos.major_hash(); // 0 — no major pieces

        pos.make_move("a7a8q"_m);
        ut::expect(ut::neq(pos.pawn_hash(),  ph0));    // pawn gone
        ut::expect(ut::eq(pos.minor_hash(),  mnh0));   // no minor involved
        ut::expect(ut::neq(pos.major_hash(), mjh0));   // queen appeared
        pos.undo_move("a7a8q"_m);
        ut::expect(ut::eq(pos.pawn_hash(),  ph0));
        ut::expect(ut::eq(pos.minor_hash(), mnh0));
        ut::expect(ut::eq(pos.major_hash(), mjh0));

        // Knight promotion: pawn_hash changes + minor_hash changes, major_hash unchanged
        pos.make_move("a7a8n"_m);
        ut::expect(ut::neq(pos.pawn_hash(),  ph0));    // pawn gone
        ut::expect(ut::neq(pos.minor_hash(), mnh0));   // knight appeared
        ut::expect(ut::eq(pos.major_hash(),  mjh0));   // no major involved
        pos.undo_move("a7a8n"_m);
        ut::expect(ut::eq(pos.pawn_hash(),  ph0));
        ut::expect(ut::eq(pos.minor_hash(), mnh0));
        ut::expect(ut::eq(pos.major_hash(), mjh0));
    };

    "pawn_hash_symmetric"_test = [] {
        // Mirror image positions must differ (not identical hash by coincidence)
        // But same position reached two ways must have equal hash
        position_t pos1{};
        pos1.make_move("e2e4"_m);
        pos1.make_move("e7e5"_m);

        position_t pos2{};
        pos2.make_move("e2e4"_m);
        pos2.make_move("e7e5"_m);

        ut::expect(ut::eq(pos1.pawn_hash(),  pos2.pawn_hash()));
        ut::expect(ut::eq(pos1.minor_hash(), pos2.minor_hash()));
        ut::expect(ut::eq(pos1.major_hash(), pos2.major_hash()));
    };
}
