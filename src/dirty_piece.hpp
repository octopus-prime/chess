#pragma once

#include <square.hpp>
#include <piece.hpp>

struct dirty_piece {
    square from;
    square to;
    piece piece;
};
