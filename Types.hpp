#pragma once

#include <cstdint>
#include <vector>

enum Player : int8_t {
    NONE = 0,
    BLACK = 1,
    WHITE = 2,
    OUT_OF_BOARD = 3
};

enum class GameMode {
    HumanVsAI,
    HumanVsHuman,
    Replay
};

// Transposition Table
enum class TTFlag : int8_t {
	EXACT,
	LOWERBOUND,
	UPPERBOUND
}; // 正確な値 α以上 β以下 


struct Move {
    int y, x;
    long long score; // 優先度スコア
    
	Move() : y(-1), x(-1), score(0) {}
	Move(int _y, int _x, long long _s = 0) : y(_y), x(_x), score(_s) {}
    bool operator>(const Move& other) const {
        return score > other.score;
    }
    bool operator==(const Move& other) const {
        return y == other.y && x == other.x;
    }
};

struct MoveResult {
	bool executed; // 非合法手対策
	std::vector<std::pair<int, int>> capturedStones; // capture復元
	uint64_t prevHash; // Zobrist完全復元
};
