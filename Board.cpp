#include "Board.hpp"
#include <cstring>

Board::Board() { reset(); }

void Board::reset()
{
    std::memset(grid, 0, sizeof(grid));
    captures[BLACK] = 0;
    captures[WHITE] = 0;
    hash = 0;
    currentTurn = BLACK;
    lastMove = {-1, -1};
}

inline bool Board::isValid(int y, int x) const
{
    return y >= 0 && y < Config::BOARD_SIZE && x >= 0 && x < Config::BOARD_SIZE;
}

inline Player Board::get(int y, int x) const
{
    if (!isValid(y, x))
        return OUT_OF_BOARD;
    return grid[y][x];
}

MoveResult Board::makeMove(int y, int x)
{
    if (grid[y][x] != NONE)
        return {false, {}, 0};

    MoveResult res;
    res.prevHash = hash;
    res.executed = true;

    grid[y][x] = currentTurn;
    hash ^= zobrist.table[y][x][currentTurn];

    Player opp = (currentTurn == BLACK) ? WHITE : BLACK;
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int i = 0; i < 8; ++i)
    {
        int y1 = y + dy[i], x1 = x + dx[i];
        int y2 = y + dy[i] * 2, x2 = x + dx[i] * 2;
        int y3 = y + dy[i] * 3, x3 = x + dx[i] * 3;

        if (get(y1, x1) == opp && get(y2, x2) == opp &&
            get(y3, x3) == currentTurn)
        {
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

void Board::undoMove(int y, int x, const MoveResult &res)
{
    Player prevPlayer = (currentTurn == BLACK) ? WHITE : BLACK;

    for (auto &p : res.capturedStones)
    {
        grid[p.first][p.second] = currentTurn;
        captures[prevPlayer] -= 1;
    }

    grid[y][x] = NONE;

    hash = res.prevHash;
    currentTurn = prevPlayer;
}

bool Board::checkWin(Player p) {
	if (captures[p] >= 10) return true;

	int dy[] = {0, 1, 1, 1};
	int dx[] = {1, 0, 1, -1};

	for (int y = 0; y < Config::BOARD_SIZE; ++y) {
		for (int x = 0; x < Config::BOARD_SIZE; ++x) {
			if (grid[y][x] != p) continue;

			for (int i = 0; i < 4; ++i) {
				// 重複チェックを避けるため、始点のみを処理
				if (get(y - dy[i], x - dx[i]) == p) continue;

				std::vector<std::pair<int, int>> line;
				int count = 0;
				int ty = y, tx = x;
				while (get(ty, tx) == p) {
					line.push_back({ty, tx});
					count++;
					ty += dy[i];
					tx += dx[i];
				}
				if (count >= 5) {
                    // 5連を発見。
                    // しかし、「相手がこのラインを破壊できるか」をチェックする
					if (!canBeBroken(p, line)) {
						return true; // 破壊できない確定した5連があれば勝利
					}
					// 破壊できる場合、このラインでの勝利は成立しない（ゲーム続行）
                }
            }
        }
    }
    return false;
}

// 相手が次の手でこのラインの一部をキャプチャし、かつ
// 「相手が勝利(10個)」するか「ラインが5個未満になる」なら true を返す
bool Board::canBeBroken(Player p, const std::vector<std::pair<int, int>>& line) {
	Player opp = (p == BLACK) ? WHITE : BLACK;
	int dy8[] = {-1, -1, -1, 0, 0, 1, 1, 1};
	int dx8[] = {-1, 0, 1, -1, 1, -1, 0, 1};

	// ライン上の全ての石について、キャプチャされるリスクがあるか調べる
	for (auto& stone : line) {
		int y = stone.first;
		int x = stone.second;

		// 石の周囲8方向をチェック
		for (int i = 0; i < 8; ++i) {
			if (get(y - dy8[i], x - dx8[i]) == opp &&
				get(y + dy8[i], x + dx8[i]) == p &&
				get(y + dy8[i] * 2, x + dx8[i] * 2) == NONE) 
			{
				if (captures[opp] + 2 >= 10) return true;
				if (line.size() - 2 < 5) return true; 
            }
        }
    }
    return false; // どの箇所もキャプチャできない、またはキャプチャされても勝ちが揺るがない
}

bool Board::isDoubleThree(int y, int x) {
	if (currentTurn != BLACK) return false;
	grid[y][x] = BLACK;
	int freeThreeCount = 0;
	int dy[] = {0, 1, 1, 1}, dx[] = {1, 0, 1, -1};
	for (int i = 0; i < 4; ++i) if (checkFreeThree(y, x, dy[i], dx[i])) freeThreeCount++;
	grid[y][x] = NONE;
	return freeThreeCount >= 2;
}

// 飛び三対応版 checkFreeThree
bool Board::checkFreeThree(int y, int x, int dy, int dx) {
	int pattern[9];
	for (int k = -4; k <= 4; ++k) {
		Player p = get(y + dy * k, x + dx * k);
		pattern[k + 4] = (p == currentTurn || (k==0)) ? 1 : (p == NONE ? 0 : 2);
	}
	// 連続3 (.XXX.)
	if (pattern[3]==0 && pattern[4]==1 && pattern[5]==1 && pattern[6]==1 && pattern[7]==0) return true;
	if (pattern[2]==0 && pattern[3]==1 && pattern[4]==1 && pattern[5]==1 && pattern[6]==0) return true;
	if (pattern[1]==0 && pattern[2]==1 && pattern[3]==1 && pattern[4]==1 && pattern[5]==0) return true;
	// 飛び3 (.X.XX. / .XX.X.)
	if (pattern[5]==0 && pattern[6]==1 && pattern[7]==1 && pattern[3]==0 && pattern[8]==0) return true;
	if (pattern[5]==1 && pattern[6]==0 && pattern[7]==1 && pattern[3]==0 && pattern[8]==0) return true;
	if (pattern[3]==0 && pattern[2]==1 && pattern[1]==1 && pattern[5]==0 && pattern[0]==0) return true;
	if (pattern[3]==1 && pattern[2]==0 && pattern[1]==1 && pattern[5]==0 && pattern[0]==0) return true;
	// 中飛び (.X.X.X.)
	if (pattern[3]==0 && pattern[2]==1 && pattern[5]==0 && pattern[6]==1 && pattern[1]==0 && pattern[7]==0) return true;

	return false;
}
