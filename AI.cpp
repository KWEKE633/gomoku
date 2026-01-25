#include "AI.hpp"
#include <algorithm>
#include <iostream>

AI::AI() : nodesVisited(0), timeOut(false)
{
	std::memset(history, 0, sizeof(history));
}

Move AI::getBestMove(Board &board, int maxDepth)
{
	tt.clear();
	std::memset(history, 0, sizeof(history));
	nodesVisited = 0;
	timeOut = false;
	startTime = std::chrono::steady_clock::now();

	Move bestMove = {-1, -1};

	for (int depth = 2; depth <= maxDepth; depth += 2)
	{
		Move m = minimaxRoot(board, depth);
		if (timeOut)
			break;
		bestMove = m;

		// 必勝状態なら早期終了
		if (bestMove.score >= Config::Score::SCORE_WIN - 10000)
			break;

		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = now - startTime;
		if (elapsed.count() > Config::TIME_LIMIT_SEC * 0.6)
			break;
	}

	std::cout << "AI Depth: " << maxDepth << " Nodes: " << nodesVisited << " Score: " << bestMove.score << std::endl;
	return bestMove;
}

Move AI::minimaxRoot(Board &board, int depth)
{
	// ルートでは候補手を生成し、高評価順に並べる
	std::vector<Move> moves = generateMoves(board);
	if (moves.empty())
		return {Config::BOARD_SIZE / 2, Config::BOARD_SIZE / 2};

	Move bestMove = moves[0];
	int alpha = -INT_MAX;
	int beta = INT_MAX;
	int bestScore = -INT_MAX;

	for (auto &m : moves)
	{
		auto res = board.makeMove(m.y, m.x);
		// 自分の手番で呼び出すので、次は相手(-negamax)
		int score = -negamax(board, depth - 1, -beta, -alpha);
		board.undoMove(m.y, m.x, res);

		if (timeOut)
			return bestMove;

		if (score > bestScore)
		{
			bestScore = score;
			bestMove = m;
			bestMove.score = score;
		}
		if (score > alpha)
		{
			alpha = score;
		}
		// ルートではBetaカットはないが、必勝手が見つかったら終わっても良い
		if (alpha >= Config::Score::SCORE_WIN - 10000)
			break;
	}
	return bestMove;
}

int AI::negamax(Board &board, int depth, int alpha, int beta)
{
	nodesVisited++;
	if ((nodesVisited & 2047) == 0)
	{
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = now - startTime;
		if (elapsed.count() > Config::TIME_LIMIT_SEC)
		{
			timeOut = true;
			return 0;
		}
	}

	// 1. TT Lookup
	if (tt.count(board.hash))
	{
		const auto &entry = tt[board.hash];
		if (entry.depth >= depth)
		{
			if (entry.flag == TTFlag::EXACT)
				return entry.score;
			if (entry.flag == TTFlag::LOWERBOUND)
				alpha = std::max(alpha, entry.score);
			if (entry.flag == TTFlag::UPPERBOUND)
				beta = std::min(beta, entry.score);
			if (alpha >= beta)
				return entry.score;
		}
	}

	// 2. 終了判定 (相手が勝ったか？)
	Player prevP = (board.currentTurn == BLACK) ? WHITE : BLACK;
	if (board.checkWin(prevP))
	{
		return -(Config::Score::SCORE_WIN + depth); // 最短で勝つ/負ける手を優先
	}

	if (depth == 0)
	{
		return evaluate(board);
	}

	// 3. 候補手生成
	std::vector<Move> moves = generateMoves(board);
	if (moves.empty())
		return 0; // Draw or No moves

	// TT Move Ordering
	if (tt.count(board.hash))
	{
		Move bestTT = tt[board.hash].bestMove;
		for (auto &m : moves)
		{
			if (m == bestTT)
			{
				m.score = LLONG_MAX; // 最優先
				break;
			}
		}
		std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
				  { return a.score > b.score; });
	}

	int originalAlpha = alpha;
	Move bestMoveInNode = {-1, -1};
	int maxScore = -INT_MAX;

	for (auto &m : moves)
	{
		auto res = board.makeMove(m.y, m.x);
		int score = -negamax(board, depth - 1, -beta, -alpha);
		board.undoMove(m.y, m.x, res);

		if (timeOut)
			return 0;

		if (score > maxScore)
		{
			maxScore = score;
			bestMoveInNode = m;
		}

		if (maxScore > alpha)
		{
			alpha = maxScore;
		}
		if (alpha >= beta)
		{
			// Cutoff
			history[m.y][m.x] += depth * depth;
			break;
		}
	}

	TTEntry entry;
	entry.key = board.hash;
	entry.depth = depth;
	entry.score = maxScore;
	entry.bestMove = bestMoveInNode;
	if (maxScore <= originalAlpha)
		entry.flag = TTFlag::UPPERBOUND;
	else if (maxScore >= beta)
		entry.flag = TTFlag::LOWERBOUND;
	else
		entry.flag = TTFlag::EXACT;

	tt[board.hash] = entry;

	return maxScore;
}

// 盤面全体の評価
int AI::evaluate(Board &board)
{
	Player me = board.currentTurn;
	Player opp = (me == BLACK) ? WHITE : BLACK;

	int score = 0;
	score += board.captures[me] * Config::Score::SCORE_CAPTURE;
	score -= board.captures[opp] * Config::Score::SCORE_CAPTURE;

	// パターン評価
	score += evaluatePattern(board, me);
	// 相手のパターンは高めに減点（防御重視）
	score -= evaluatePattern(board, opp) * Config::Score::DEF_BIAS;

	return score;
}

// パターン評価（4連、3連など）
int AI::evaluatePattern(Board &board, Player p)
{
	int score = 0;
	int dy[] = {0, 1, 1, 1};
	int dx[] = {1, 0, 1, -1};

	// 高速化のため、石がある場所のみ評価したいが、
	// 簡易実装として全マス走査する（19x19x4なら十分高速）
	for (int y = 0; y < Config::BOARD_SIZE; ++y)
	{
		for (int x = 0; x < Config::BOARD_SIZE; ++x)
		{
			if (board.grid[y][x] != p)
				continue;

			for (int dir = 0; dir < 4; ++dir)
			{
				// 重複カウント防止：逆方向が同じ色ならスキップ（既にカウント済み）
				if (board.get(y - dy[dir], x - dx[dir]) == p)
					continue;

				int len = 0;
				int cy = y, cx = x;
				while (board.get(cy, cx) == p)
				{
					len++;
					cy += dy[dir];
					cx += dx[dir];
				}

				bool openHead = (board.get(y - dy[dir], x - dx[dir]) == NONE);
				bool openTail = (board.get(cy, cx) == NONE);

				if (len >= 5)
				{
					score += Config::Score::SCORE_WIN;
				}
				else if (len == 4)
				{
					if (openHead && openTail)
						score += Config::Score::SCORE_OPEN4; // 止められない
					else if (openHead || openTail)
						score += Config::Score::SCORE_CLOSED4; // 止めないとやばい
				}
				else if (len == 3)
				{
					if (openHead && openTail)
						score += Config::Score::SCORE_OPEN3; // 次にOpen4になるので非常に危険
					else if (openHead || openTail)
						score += Config::Score::SCORE_CLOSED3;
				}
				else if (len == 2)
				{
					if (openHead && openTail)
						score += Config::Score::SCORE_OPEN2;
				}
			}
		}
	}
	return score;
}

// 候補手生成 & 優先度付きソート
std::vector<Move> AI::generateMoves(Board &board)
{
	std::vector<Move> moves;
	// 探索範囲: 石がある場所の近傍2マス以内
	bool visited[Config::BOARD_SIZE][Config::BOARD_SIZE] = {};

	// 攻撃と防御の重要ポイントを簡易計算するためのヘルパー
	auto evalPoint = [&](int y, int x, Player p) -> long long
	{
		long long s = 0;
		int dy[] = {0, 1, 1, 1};
		int dx[] = {1, 0, 1, -1};
		for (int d = 0; d < 4; ++d)
		{
			int len = 1;
			// 正方向
			int ty = y + dy[d], tx = x + dx[d];
			while (board.get(ty, tx) == p)
			{
				len++;
				ty += dy[d];
				tx += dx[d];
			}
			// 負方向
			ty = y - dy[d];
			tx = x - dx[d];
			while (board.get(ty, tx) == p)
			{
				len++;
				ty -= dy[d];
				tx -= dx[d];
			}

			// 簡易スコア: 長さの指数関数的重み
			if (len >= 5)
				s += 1000000;
			else if (len == 4)
				s += 100000;
			else if (len == 3)
				s += 10000;
			else if (len == 2)
				s += 100;
		}
		return s;
	};

	Player me = board.currentTurn;
	Player opp = (me == BLACK ? WHITE : BLACK);

	for (int y = 0; y < Config::BOARD_SIZE; ++y)
	{
		for (int x = 0; x < Config::BOARD_SIZE; ++x)
		{
			if (board.grid[y][x] != NONE)
			{
				for (int dy = -2; dy <= 2; ++dy)
				{
					for (int dx = -2; dx <= 2; ++dx)
					{
						int ny = y + dy;
						int nx = x + dx;
						if (board.isValid(ny, nx) && board.grid[ny][nx] == NONE && !visited[ny][nx])
						{
							visited[ny][nx] = true;

							if (me == BLACK && board.isDoubleThree(ny, nx))
								continue;

							long long prio = 0;
							// 1. 履歴
							prio += history[ny][nx];
							// 2. 中央寄せ
							prio += (10 - abs(ny - 9) - abs(nx - 9)) * 10;

							// 3. 局所評価（攻撃・防御）
							// 自分の攻撃手としての価値
							long long atk = evalPoint(ny, nx, me);
							// 相手の攻撃を防ぐ価値（防御）
							long long def = evalPoint(ny, nx, opp);

							// 防御の重みを攻撃より少し高くすることで、危機を見逃さない
							prio += atk * 10;
							prio += def * 12;

							moves.push_back({ny, nx, prio});
						}
					}
				}
			}
		}
	}

	// スコア順にソート（重要）
	std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
			  { return a.score > b.score; });

	// Beam Width制限（ただし重要な手が漏れないよう幅を広げる）
	if (moves.size() > Config::BEAM_WIDTH)
	{
		moves.resize(Config::BEAM_WIDTH);
	}

	return moves;
}
