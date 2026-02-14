#pragma once

#include "Config.hpp"
#include "Types.hpp"
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
	// 勝利判定（即時勝利判定含む）
	bool checkWin(Player p);
    bool isDoubleThree(int y, int x);
    bool isValid(int y, int x) const;
    Player get(int y, int x) const;
	// 指定されたラインが相手の次の手でキャプチャされ、かつ
	// 「相手の勝利(10個)」または「5連の破壊」につながるか判定する
	bool canBeBroken(Player p, const std::vector<std::pair<int, int>>& line);

  private:
    bool checkFreeThree(int y, int x, int dy, int dx);
};
