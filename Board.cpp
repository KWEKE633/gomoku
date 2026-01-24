#include "Board.hpp"
#include <cstring>

Board::Board() {
	reset();
}

void Board::reset() {
	std::memset(grid, 0, sizeof(grid));
	captures[BLACK] = 0;
	captures[WHITE] = 0;
	hash = 0;
	currentTurn = BLACK;
	lastMove = {-1, -1};
}

inline bool Board::isValid(int y, int x) const {
	return y >= 0 && y < Config::BOARD_SIZE && x >= 0 && x < Config::BOARD_SIZE;
}

inline Player Board::get(int y, int x) const {
	if (!isValid(y, x)) return OUT_OF_BOARD;
	return grid[y][x];
}

MoveResult Board::makeMove(int y, int x) {
	if (grid[y][x] != NONE) return {false, {}, 0};

	MoveResult res;
	res.prevHash = hash;
	res.executed = true;

	grid[y][x] = currentTurn;
	hash ^= zobrist.table[y][x][currentTurn];

	Player opp = (currentTurn == BLACK) ? WHITE : BLACK;
	int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
	int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};

	for (int i = 0; i < 8; ++i) {
		int y1 = y + dy[i], x1 = x + dx[i];
		int y2 = y + dy[i]*2, x2 = x + dx[i]*2;
		int y3 = y + dy[i]*3, x3 = x + dx[i]*3;

		if (get(y1, x1) == opp && get(y2, x2) == opp && get(y3, x3) == currentTurn) {
			grid[y1][x1] = NONE;
			grid[y2][x2] = NONE;
			
			hash ^= zobrist.table[y1][x1][opp];
			hash ^= zobrist.table[y2][x2][opp];

			captures[currentTurn] += 2;
			res.capturedStones.push_back({y1, x1});
			res.capturedStones.push_back({y2, x2});
		}
	}

	hash ^= zobrist.turnHash;
	currentTurn = opp;
	lastMove = {y, x};
	return res;
}

void Board::undoMove(int y, int x, const MoveResult& res) {
	Player prevPlayer = (currentTurn == BLACK) ? WHITE : BLACK;
	
	for (auto& p : res.capturedStones) {
		grid[p.first][p.second] = currentTurn;
		captures[prevPlayer] -= 1;
	}

	grid[y][x] = NONE;
	
	hash = res.prevHash;
	currentTurn = prevPlayer;
}

bool Board::checkWin(Player p, bool checkCanBreak) {
	if (captures[p] >= 10) return true;

	int dy[] = {0, 1, 1, 1};
	int dx[] = {1, 0, 1, -1};

	for (int y = 0; y < Config::BOARD_SIZE; ++y) {
		for (int x = 0; x < Config::BOARD_SIZE; ++x) {
			if (grid[y][x] != p) continue;

			for (int i = 0; i < 4; ++i) {
				int count = 1;
				int ty = y + dy[i], tx = x + dx[i];
				while (get(ty, tx) == p) { count++; ty += dy[i]; tx += dx[i]; }
				
				if (count >= 5) {
					if (!checkCanBreak) return true;
					return true; 
				}
			}
		}
	}
	return false;
}

// 禁じ手チェック (黒番のみ: 3-3)
bool Board::isDoubleThree(int y, int x) {
	if (currentTurn != BLACK) return false;
	
	grid[y][x] = BLACK;
	int freeThreeCount = 0;

	int dy[] = {0, 1, 1, 1};
	int dx[] = {1, 0, 1, -1};

	for (int i = 0; i < 4; ++i) {
		if (checkFreeThree(y, x, dy[i], dx[i])) {
			freeThreeCount++;
		}
	}
	grid[y][x] = NONE;

	return (freeThreeCount >= 2);
}

bool Board::checkFreeThree(int y, int x, int dy, int dx) {
	int count = 1;
	int tY, tX;

	// 正方向
	tY = y + dy; tX = x + dx;
	while (get(tY, tX) == BLACK) { count++; tY += dy; tX += dx; }
	int space1Y = tY, space1X = tX;

	// 負方向
	tY = y - dy; tX = x - dx;
	while (get(tY, tX) == BLACK) { count++; tY -= dy; tX -= dx; }
	int space2Y = tY, space2X = tX;

	if (count == 3) {
		// 両端が空いているか
		if (get(space1Y, space1X) == NONE && get(space2Y, space2X) == NONE) {
			return true;
		}
	}
	return false;
}
