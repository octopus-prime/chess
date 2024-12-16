#pragma once

#include <piece.hpp>

static_assert(sizeof(piece) == 1);
static_assert(piece{wpawn} == wpawn);
static_assert(piece{wking} == wking);
static_assert(piece{bpawn} == bpawn);
static_assert(piece{bking} == bking);
static_assert(piece{white, pawn} == wpawn);
static_assert(piece{black, pawn} == bpawn);
static_assert(piece{wpawn}.side() == white);
static_assert(piece{wpawn}.type() == pawn);
static_assert(piece{wpawn} + 1 == wknight);
