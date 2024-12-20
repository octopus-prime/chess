#pragma once

#include <piece.hpp>

static_assert(sizeof(piece) == 1);
static_assert(piece{WPAWN} == WPAWN);
static_assert(piece{WKING} == WKING);
static_assert(piece{BPAWN} == BPAWN);
static_assert(piece{BKING} == BKING);
static_assert(piece{WHITE, PAWN} == WPAWN);
static_assert(piece{BLACK, PAWN} == BPAWN);
static_assert(piece{WPAWN}.side() == WHITE);
static_assert(piece{WPAWN}.type() == PAWN);
static_assert(piece{WPAWN} + 1 == WKNIGHT);
