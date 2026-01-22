//　42の環境で動かすにはSFML ver2.5に調整しなければいけない
//（今回のDockerfileとLinux的に）

#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#include <climits>
#include <string>
#include <cstring>
#include <chrono>
#include <random>
#include <unordered_map>
#include <optional>
#include <array>


constexpr int BOARD_SIZE = 19;
constexpr int CELL_SIZE  = 35;
constexpr int OFFSET     = 40;
constexpr unsigned int WINDOW_W = 750;
constexpr unsigned int WINDOW_H = 850;

// AI
constexpr double TIME_LIMIT_SEC = 0.48;
constexpr int MAX_DEPTH         = 10;
constexpr int BEAM_WIDTH        = 30; // 候補手を広げて見落としを防ぐ

// Colors
const sf::Color COLOR_BG(222, 184, 135);
const sf::Color COLOR_LINE(0, 0, 0, 200);
const sf::Color COLOR_TEXT(20, 20, 20);


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

struct Move {
    int y, x;
    long long score = 0; // 優先度スコア
    
    bool operator>(const Move& other) const {
        return score > other.score;
    }
    bool operator==(const Move& other) const {
        return y == other.y && x == other.x;
    }
};


class Zobrist { // ボードゲーム用に考案されたハッシュ手法の名前 同じ局面を何度も評価してしまうことを防ぐ
public:
    uint64_t table[BOARD_SIZE][BOARD_SIZE][3]; // [y][x][player]
    uint64_t turnHash;

    Zobrist() {
        std::mt19937_64 rng(12345); //　周期と範囲が広いため、ハッシュ化に適している
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                for (int p = 0; p < 3; ++p) {
                    table[y][x][p] = rng();
                }
            }
        }
        turnHash = rng();
    }
};

static Zobrist zobrist;

// Transposition Table
enum class TTFlag : int8_t { EXACT, LOWERBOUND, UPPERBOUND }; // 正確な値 α以上 β以下 

struct TTEntry {
    uint64_t key;
    int depth;
    int score;
    TTFlag flag;
    Move bestMove;
};

// Game Logic & Board State
class Board {
public:
    Player grid[BOARD_SIZE][BOARD_SIZE];
    int captures[3]; // 1=Black, 2=White
    uint64_t hash;
    Player currentTurn;
    
    Move lastMove = {-1, -1}; // 赤点で最後に置かれた石を表示するための座標

    Board() {
        reset();
    }

    void reset() {
        std::memset(grid, 0, sizeof(grid));
        captures[BLACK] = 0;
        captures[WHITE] = 0;
        hash = 0;
        currentTurn = BLACK;
        lastMove = {-1, -1};
    }

    inline bool isValid(int y, int x) const {
        return y >= 0 && y < BOARD_SIZE && x >= 0 && x < BOARD_SIZE;
    }

    inline Player get(int y, int x) const {
        if (!isValid(y, x)) return OUT_OF_BOARD;
        return grid[y][x];
    }

    struct MoveResult {
        bool executed; // 非合法手対策
        std::vector<std::pair<int, int>> capturedStones; // capture復元
        uint64_t prevHash; // Zobrist完全復元
    };

    MoveResult makeMove(int y, int x) {
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

    void undoMove(int y, int x, const MoveResult& res) {
        Player prevPlayer = (currentTurn == BLACK) ? WHITE : BLACK;
        
        for (auto& p : res.capturedStones) {
            grid[p.first][p.second] = currentTurn;
            captures[prevPlayer] -= 1;
        }

        grid[y][x] = NONE;
        
        hash = res.prevHash;
        currentTurn = prevPlayer;
    }

    bool checkWin(Player p, bool checkCanBreak = true) {
        if (captures[p] >= 10) return true;

        int dy[] = {0, 1, 1, 1};
        int dx[] = {1, 0, 1, -1};

        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
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
    bool isDoubleThree(int y, int x) {
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

private:
    bool checkFreeThree(int y, int x, int dy, int dx) {
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
};


// AI Engine

class AI {
private:
    // 評価スコア定義
    static constexpr int SCORE_WIN        = 100000000;
    static constexpr int SCORE_OPEN4      = 10000000; // 止められない4
    static constexpr int SCORE_CLOSED4    = 2000000;  // 4（片側止まり）
    static constexpr int SCORE_OPEN3      = 1000000;  // Open3（次にOpen4になるので非常に危険）
    static constexpr int SCORE_CLOSED3    = 5000;
    static constexpr int SCORE_OPEN2      = 3000;
    static constexpr int SCORE_CAPTURE    = 150000;   // 捕獲価値

    std::unordered_map<uint64_t, TTEntry> tt;
    long long history[BOARD_SIZE][BOARD_SIZE];
    std::chrono::steady_clock::time_point startTime;
    
    int nodesVisited = 0;
    bool timeOut = false;

public:
    AI() {
        std::memset(history, 0, sizeof(history));
    }

    Move getBestMove(Board& board, int maxDepth = MAX_DEPTH) {
        tt.clear();
        std::memset(history, 0, sizeof(history));
        nodesVisited = 0;
        timeOut = false;
        startTime = std::chrono::steady_clock::now();

        Move bestMove = {-1, -1};
        
        for (int depth = 2; depth <= maxDepth; depth += 2) {
            Move m = minimaxRoot(board, depth);
            if (timeOut) break;
            bestMove = m;
            
            // 必勝状態なら早期終了
            if (bestMove.score >= SCORE_WIN - 10000) break;
            
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = now - startTime;
            if (elapsed.count() > TIME_LIMIT_SEC * 0.6) break;
        }

        std::cout << "AI Depth: " << maxDepth << " Nodes: " << nodesVisited << " Score: " << bestMove.score << std::endl;
        return bestMove;
    }

private:
    Move minimaxRoot(Board& board, int depth) {
        // ルートでは候補手を生成し、高評価順に並べる
        std::vector<Move> moves = generateMoves(board);
        if (moves.empty()) return {BOARD_SIZE/2, BOARD_SIZE/2};

        Move bestMove = moves[0];
        int alpha = -INT_MAX;
        int beta = INT_MAX;
        int bestScore = -INT_MAX;

        for (auto& m : moves) {
            auto res = board.makeMove(m.y, m.x);
            // 自分の手番で呼び出すので、次は相手(-negamax)
            int score = -negamax(board, depth - 1, -beta, -alpha);
            board.undoMove(m.y, m.x, res);

            if (timeOut) return bestMove;

            if (score > bestScore) {
                bestScore = score;
                bestMove = m;
                bestMove.score = score;
            }
            if (score > alpha) {
                alpha = score;
            }
            // ルートではBetaカットはないが、必勝手が見つかったら終わっても良い
            if (alpha >= SCORE_WIN - 10000) break;
        }
        return bestMove;
    }

    int negamax(Board& board, int depth, int alpha, int beta) {
        nodesVisited++;
        if ((nodesVisited & 2047) == 0) {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = now - startTime;
            if (elapsed.count() > TIME_LIMIT_SEC) {
                timeOut = true;
                return 0;
            }
        }

        // 1. TT Lookup
        if (tt.count(board.hash)) {
            const auto& entry = tt[board.hash];
            if (entry.depth >= depth) {
                if (entry.flag == TTFlag::EXACT) return entry.score;
                if (entry.flag == TTFlag::LOWERBOUND) alpha = std::max(alpha, entry.score);
                if (entry.flag == TTFlag::UPPERBOUND) beta = std::min(beta, entry.score);
                if (alpha >= beta) return entry.score;
            }
        }

        // 2. 終了判定 (相手が勝ったか？)
        Player prevP = (board.currentTurn == BLACK) ? WHITE : BLACK;
        if (board.checkWin(prevP)) {
            return -(SCORE_WIN + depth); // 最短で勝つ/負ける手を優先
        }
        
        if (depth == 0) {
            return evaluate(board);
        }

        // 3. 候補手生成
        std::vector<Move> moves = generateMoves(board);
        if (moves.empty()) return 0; // Draw or No moves

        // TT Move Ordering
        if (tt.count(board.hash)) {
            Move bestTT = tt[board.hash].bestMove;
            for (auto& m : moves) {
                if (m == bestTT) {
                    m.score = LLONG_MAX; // 最優先
                    break;
                }
            }
            std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){
                return a.score > b.score;
            });
        }

        int originalAlpha = alpha;
        Move bestMoveInNode = {-1, -1};
        int maxScore = -INT_MAX;

        for (auto& m : moves) {
            auto res = board.makeMove(m.y, m.x);
            int score = -negamax(board, depth - 1, -beta, -alpha);
            board.undoMove(m.y, m.x, res);

            if (timeOut) return 0;

            if (score > maxScore) {
                maxScore = score;
                bestMoveInNode = m;
            }
            
            if (maxScore > alpha) {
                alpha = maxScore;
            }
            if (alpha >= beta) {
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
        if (maxScore <= originalAlpha) entry.flag = TTFlag::UPPERBOUND;
        else if (maxScore >= beta) entry.flag = TTFlag::LOWERBOUND;
        else entry.flag = TTFlag::EXACT;
        
        tt[board.hash] = entry;

        return maxScore;
    }

    // 盤面全体の評価
    int evaluate(Board& board) {
        Player me = board.currentTurn;
        Player opp = (me == BLACK) ? WHITE : BLACK;
        
        int score = 0;
        score += board.captures[me] * SCORE_CAPTURE;
        score -= board.captures[opp] * SCORE_CAPTURE;
        
        // パターン評価
        score += evaluatePattern(board, me);
        // 相手のパターンは高めに減点（防御重視）
        score -= evaluatePattern(board, opp) * 1.2;

        return score;
    }

    // パターン評価（4連、3連など）
    int evaluatePattern(Board& board, Player p) {
        int score = 0;
        int dy[] = {0, 1, 1, 1};
        int dx[] = {1, 0, 1, -1};

        // 高速化のため、石がある場所のみ評価したいが、
        // 簡易実装として全マス走査する（19x19x4なら十分高速）
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (board.grid[y][x] != p) continue;

                for (int dir = 0; dir < 4; ++dir) {
                    // 重複カウント防止：逆方向が同じ色ならスキップ（既にカウント済み）
                    if (board.get(y - dy[dir], x - dx[dir]) == p) continue;

                    int len = 0;
                    int cy = y, cx = x;
                    while (board.get(cy, cx) == p) {
                        len++;
                        cy += dy[dir];
                        cx += dx[dir];
                    }
                    
                    bool openHead = (board.get(y - dy[dir], x - dx[dir]) == NONE);
                    bool openTail = (board.get(cy, cx) == NONE);

                    if (len >= 5) {
                        score += SCORE_WIN;
                    } else if (len == 4) {
                        if (openHead && openTail) score += SCORE_OPEN4; // 止められない
                        else if (openHead || openTail) score += SCORE_CLOSED4; // 止めないとやばい
                    } else if (len == 3) {
                        if (openHead && openTail) score += SCORE_OPEN3; // 次にOpen4になるので非常に危険
                        else if (openHead || openTail) score += SCORE_CLOSED3;
                    } else if (len == 2) {
                        if (openHead && openTail) score += SCORE_OPEN2;
                    }
                }
            }
        }
        return score;
    }

    // 候補手生成 & 優先度付きソート
    std::vector<Move> generateMoves(Board& board) {
        std::vector<Move> moves;
        // 探索範囲: 石がある場所の近傍2マス以内
        bool visited[BOARD_SIZE][BOARD_SIZE] = {};

        // 攻撃と防御の重要ポイントを簡易計算するためのヘルパー
        auto evalPoint = [&](int y, int x, Player p) -> long long {
             long long s = 0;
             int dy[] = {0, 1, 1, 1};
             int dx[] = {1, 0, 1, -1};
             for(int d=0; d<4; ++d) {
                 int len = 1; 
                 // 正方向
                 int ty = y + dy[d], tx = x + dx[d];
                 while(board.get(ty, tx) == p) { len++; ty += dy[d]; tx += dx[d]; }
                 // 負方向
                 ty = y - dy[d]; tx = x - dx[d];
                 while(board.get(ty, tx) == p) { len++; ty -= dy[d]; tx -= dx[d]; }
                 
                 // 簡易スコア: 長さの指数関数的重み
                 if (len >= 5) s += 1000000;
                 else if (len == 4) s += 100000;
                 else if (len == 3) s += 10000;
                 else if (len == 2) s += 100;
             }
             return s;
        };

        Player me = board.currentTurn;
        Player opp = (me == BLACK ? WHITE : BLACK);

        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (board.grid[y][x] != NONE) {
                    for (int dy = -2; dy <= 2; ++dy) {
                        for (int dx = -2; dx <= 2; ++dx) {
                            int ny = y + dy;
                            int nx = x + dx;
                            if (board.isValid(ny, nx) && board.grid[ny][nx] == NONE && !visited[ny][nx]) {
                                visited[ny][nx] = true;

                                if (me == BLACK && board.isDoubleThree(ny, nx)) continue;

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
        std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){
            return a.score > b.score;
        });

        // Beam Width制限（ただし重要な手が漏れないよう幅を広げる）
        if (moves.size() > BEAM_WIDTH) {
            moves.resize(BEAM_WIDTH);
        }

        return moves;
    }
};

// GUI & Game Manager (SFML 3.0 Compatible)

class GomokuGame {
private:
    sf::RenderWindow window;
    Board board;
    AI ai;
    
    sf::Font font;
    sf::Text statusText;
    sf::Text guideText;
    sf::Text timerText;

    GameMode mode = GameMode::HumanVsAI;
    Player userColor = BLACK;
    bool gameOver = false;
    Player winner = NONE;

    std::vector<std::pair<int, int>> moveHistory;
    int replayIndex = -1;
    bool isReplayMode = false;

public:
    GomokuGame() 
        : window(sf::VideoMode({WINDOW_W, WINDOW_H}), "42 Gomoku AI - High Defense"),
          statusText(font),
          guideText(font),
          timerText(font)
    {
        window.setFramerateLimit(60);
        loadFont();
        initText();
    }

    void run() {
        while (window.isOpen()) {
            processEvents();
            if (!isReplayMode) update();
            render();
        }
    }

private:
    void loadFont() {
        const char* fonts[] = {
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "C:\\Windows\\Fonts\\arial.ttf",
            "Arial.ttf"
        };
        for (auto path : fonts) {
            if (font.openFromFile(path)) return;
        }
        std::cerr << "Warning: No font found." << std::endl;
    }

    void initText() {
        statusText.setFont(font);
        statusText.setCharacterSize(24);
        statusText.setFillColor(COLOR_TEXT);
        statusText.setPosition({20.f, (float)WINDOW_H - 90.f});

        guideText.setFont(font);
        guideText.setCharacterSize(16);
        guideText.setFillColor(sf::Color(50, 50, 50));
        guideText.setPosition({20.f, (float)WINDOW_H - 50.f});
        guideText.setString("Keys: [1]PvAI [2]PvP [3]SwapColor [L]Replay [R]Reset [ESC]Quit");

        timerText.setFont(font);
        timerText.setCharacterSize(20);
        timerText.setFillColor(sf::Color::Red);
        timerText.setPosition({(float)WINDOW_W - 150.f, (float)WINDOW_H - 90.f});
    }

    void processEvents() {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Escape) window.close();
                if (keyEvent->code == sf::Keyboard::Key::R) resetGame();
                
                if (!isReplayMode) {
                    if (keyEvent->code == sf::Keyboard::Key::Num1) { mode = GameMode::HumanVsAI; resetGame(); }
                    if (keyEvent->code == sf::Keyboard::Key::Num2) { mode = GameMode::HumanVsHuman; resetGame(); }
                    if (keyEvent->code == sf::Keyboard::Key::Num3) { userColor = (userColor == BLACK ? WHITE : BLACK); resetGame(); }
                    if (keyEvent->code == sf::Keyboard::Key::L) startReplay();
                } else {
                    if (keyEvent->code == sf::Keyboard::Key::L) stopReplay();
                    if (keyEvent->code == sf::Keyboard::Key::Right) replayNext();
                    if (keyEvent->code == sf::Keyboard::Key::Left) replayPrev();
                }
            }

            if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseEvent->button == sf::Mouse::Button::Left) {
                    if (!isReplayMode && !gameOver) {
                        if (mode == GameMode::HumanVsAI && board.currentTurn != userColor) return;
                        handleMouseClick(mouseEvent->position.x, mouseEvent->position.y);
                    }
                }
            }
        }
    }

    void handleMouseClick(int mx, int my) {
        if (mx < OFFSET || my < OFFSET) return;
        int x = (mx - OFFSET + CELL_SIZE/2) / CELL_SIZE;
        int y = (my - OFFSET + CELL_SIZE/2) / CELL_SIZE;

        if (board.isValid(y, x)) {
            if (board.currentTurn == BLACK && board.isDoubleThree(y, x)) {
                statusText.setString("Forbidden Move (Double-Three)!");
                return;
            }
            if (board.get(y, x) == NONE) {
                doMove(y, x);
            }
        }
    }

    void doMove(int y, int x) {
        moveHistory.push_back({y, x});
        board.makeMove(y, x);
        
        Player justMoved = (board.currentTurn == BLACK) ? WHITE : BLACK;
        if (board.checkWin(justMoved, true)) {
            gameOver = true;
            winner = justMoved;
            statusText.setString(std::string(winner == BLACK ? "Black" : "White") + " Wins!");
        } else {
            updateStatusText();
        }
    }

    void update() {
        if (gameOver) return;

        if (mode == GameMode::HumanVsAI && board.currentTurn != userColor) {
            statusText.setString("AI Thinking...");
            window.draw(statusText);
            window.display();

            sf::Clock clock;
            Move bestMove = ai.getBestMove(board);
            float time = clock.getElapsedTime().asSeconds();
            timerText.setString(std::to_string(time).substr(0, 4) + "s");

            if (bestMove.y != -1) {
                doMove(bestMove.y, bestMove.x);
            } else {
                gameOver = true;
                statusText.setString("AI Resigns. You Win!");
            }
        }
    }

    void render() {
        window.clear(COLOR_BG);

        for (int i = 0; i < BOARD_SIZE; ++i) {
            sf::RectangleShape hline(sf::Vector2f(CELL_SIZE * (BOARD_SIZE - 1), 2));
            hline.setFillColor(COLOR_LINE);
            hline.setPosition({(float)OFFSET, (float)(OFFSET + i * CELL_SIZE)});
            window.draw(hline);

            sf::RectangleShape vline(sf::Vector2f(2, CELL_SIZE * (BOARD_SIZE - 1)));
            vline.setFillColor(COLOR_LINE);
            vline.setPosition({(float)(OFFSET + i * CELL_SIZE), (float)OFFSET});
            window.draw(vline);
        }

        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                Player p = board.get(y, x);
                if (p != NONE) {
                    drawStone(y, x, p == BLACK ? sf::Color::Black : sf::Color::White);
                }
            }
        }

        if (board.lastMove.y != -1) {
            sf::CircleShape mark(4);
            mark.setOrigin({4, 4});
            mark.setFillColor(sf::Color::Red);
            mark.setPosition({(float)(OFFSET + board.lastMove.x * CELL_SIZE), (float)(OFFSET + board.lastMove.y * CELL_SIZE)});
            window.draw(mark);
        }

        window.draw(statusText);
        window.draw(guideText);
        window.draw(timerText);

        sf::Text capsInfo(font);
        capsInfo.setCharacterSize(18);
        capsInfo.setFillColor(COLOR_TEXT);
        capsInfo.setPosition({(float)WINDOW_W - 200.f, 50.f});
        capsInfo.setString("Captures:\nBlack: " + std::to_string(board.captures[BLACK]) + "/10\nWhite: " + std::to_string(board.captures[WHITE]) + "/10");
        window.draw(capsInfo);

        window.display();
    }

    void drawStone(int y, int x, sf::Color c) {
        sf::CircleShape stone(CELL_SIZE / 2 - 2);
        stone.setOrigin({(float)(CELL_SIZE / 2 - 2), (float)(CELL_SIZE / 2 - 2)});
        stone.setFillColor(c);
        stone.setPosition({(float)(OFFSET + x * CELL_SIZE), (float)(OFFSET + y * CELL_SIZE)});
        stone.setOutlineThickness(1);
        stone.setOutlineColor(sf::Color(50, 50, 50));
        window.draw(stone);
    }

    void updateStatusText() {
        std::string turnStr = (board.currentTurn == BLACK) ? "Black" : "White";
        statusText.setString("Turn: " + turnStr + (isReplayMode ? " [REPLAY]" : ""));
    }

    void resetGame() {
        board.reset();
        moveHistory.clear();
        gameOver = false;
        winner = NONE;
        isReplayMode = false;
        updateStatusText();
        timerText.setString("");
    }

    void startReplay() {
        isReplayMode = true;
        replayIndex = (int)moveHistory.size();
        updateStatusText();
    }
    
    void stopReplay() {
        isReplayMode = false;
        while (replayIndex < (int)moveHistory.size()) replayNext();
        updateStatusText();
    }

    void replayPrev() {
        if (replayIndex > 0) {
            replayIndex--;
            reconstructBoard(replayIndex);
        }
    }

    void replayNext() {
        if (replayIndex < (int)moveHistory.size()) {
            replayIndex++;
            reconstructBoard(replayIndex);
        }
    }

    void reconstructBoard(int untilIndex) {
        board.reset();
        for (int i = 0; i < untilIndex; ++i) {
            board.makeMove(moveHistory[i].first, moveHistory[i].second);
        }
    }
};

int main() {
    GomokuGame game;
    game.run();
    return 0;
}
