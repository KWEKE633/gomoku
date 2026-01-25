#pragma once

#include "Types.hpp"
#include "Config.hpp"
#include "Zobrist.hpp"
#include <vector>

class Board
{
public:
	Player grid[Config::BOARD_SIZE][Config::BOARD_SIZE];
	int captures[3];
	uint64_t hash;
	Player currentTurn;
	Move lastMove;

	Board();
	void reset();
	MoveResult makeMove(int y, int x);
	void undoMove(int y, int x, const MoveResult &res);
	bool checkWin(Player p, bool checkCanBreak = true);
	bool isDoubleThree(int y, int x);
	bool isValid(int y, int x) const;
	Player get(int y, int x) const;

private:
	bool checkFreeThree(int y, int x, int dy, int dx);
};
