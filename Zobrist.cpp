#include "Zobrist.hpp"
#include <random>

Zobrist::Zobrist()
{
    std::mt19937_64 rng(
        12345); // 　周期と範囲が広いため、ハッシュ化に適している
    for (int y = 0; y < Config::BOARD_SIZE; ++y)
    {
        for (int x = 0; x < Config::BOARD_SIZE; ++x)
        {
            for (int p = 0; p < 3; ++p)
            {
                table[y][x][p] = rng();
            }
        }
    }
    turnHash = rng();
}

const Zobrist zobrist;
