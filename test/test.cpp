#include "test_square.hpp"
#include "test_side.hpp"
#include "test_piece.hpp"
#include "test_bitboard.hpp"
#include "test_bitboards.hpp"

#include "nnue/test_affine_tranform.hpp"
#include "nnue/test_clipped_relu.hpp"
#include "nnue/test_mul_clipped_relu.hpp"
#include "nnue/test_sqr_clipped_relu.hpp"
#include "nnue/test_nnue.hpp"

int main() {
    test_square();
    test_bitboard();
    test_bitboards();

    nnue::test_clipped_relu_16();
    nnue::test_clipped_relu_32();
    nnue::test_sqr_clipped_relu_16();
    nnue::test_mul_clipped_relu_64();
    nnue::test_affine_tranform_32_1();
    nnue::test_affine_tranform_32_32();
    nnue::test_affine_tranform_32_32_2();
    nnue::test_nnue();

    // demo_bitboard();
}