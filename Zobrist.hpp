#pragma once

#include "Config.hpp"
#include <cstdint>

class Zobrist {
public:
    uint64_t table[Config::BOARD_SIZE][Config::BOARD_SIZE][3];
    uint64_t turnHash;

    Zobrist();
};

extern const Zobrist zobrist;
