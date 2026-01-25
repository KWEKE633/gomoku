#pragma once

#include "Board.hpp"
#include <unordered_map>
#include <chrono>
#include <cstring>
#include <climits>

struct TTEntry
{
	uint64_t key;
	int depth;
	int score;
	TTFlag flag;
	Move bestMove;
};

// AI Engine

class AI
{
public:
	AI();
	Move getBestMove(Board &board, int maxDepth = Config::MAX_DEPTH);

private:
	Move minimaxRoot(Board &board, int depth);

	int negamax(Board &board, int depth, int alpha, int beta);

	// 盤面全体の評価
	int evaluate(Board &board);

	// パターン評価（4連、3連など）
	int evaluatePattern(Board &board, Player p);

	// 候補手生成 & 優先度付きソート
	std::vector<Move> generateMoves(Board &board);

	std::unordered_map<uint64_t, TTEntry> tt;
	long long history[Config::BOARD_SIZE][Config::BOARD_SIZE];
	std::chrono::steady_clock::time_point startTime;

	int nodesVisited;
	bool timeOut;
};
