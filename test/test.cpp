#include "test_square.hpp"
#include "test_side.hpp"
#include "test_piece.hpp"
#include "test_bitboard.hpp"
#include "test_bitboards.hpp"
#include "test_move.hpp"
#include "test_position.hpp"
#include "test_perft.hpp"
#include "test_history.hpp"

#include "nnue/test_affine_tranform.hpp"
#include "nnue/test_clipped_relu.hpp"
#include "nnue/test_mul_clipped_relu.hpp"
#include "nnue/test_sqr_clipped_relu.hpp"
#include "nnue/test_nnue.hpp"



int main() {
    test_square();
    test_bitboard();
    test_bitboards();
    test_move();
    test_position();
    test_perft();
    test_history();

    nnue::test_clipped_relu();
    nnue::test_sqr_clipped_relu();
    nnue::test_mul_clipped_relu();
    nnue::test_affine_tranform();
    nnue::test_nnue();

    // demo_bitboard();
}