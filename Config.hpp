#pragma once
#include <SFML/Graphics.hpp>

namespace Config
{
// Board
constexpr int BOARD_SIZE = 19;
constexpr int CELL_SIZE = 35;
constexpr int OFFSET = 40;

// Window
constexpr unsigned int WINDOW_W = 750;
constexpr unsigned int WINDOW_H = 850;

// AI Performance
constexpr double TIME_LIMIT_SEC = 0.48;
constexpr int MAX_DEPTH = 10;
constexpr int BEAM_WIDTH = 30;

// AI Scores
namespace Score
{
constexpr int SCORE_WIN = 100000000;
constexpr int SCORE_OPEN4 = 10000000;  // 止められない4
constexpr int SCORE_CLOSED4 = 2000000; // 4（片側止まり）
constexpr int SCORE_OPEN3 = 1000000;   // Open3（次にOpen4になるので非常に危険）
constexpr int SCORE_CLOSED3 = 5000;
constexpr int SCORE_OPEN2 = 3000;
constexpr int SCORE_CAPTURE = 150000; // 捕獲価値
constexpr double DEF_BIAS = 1.2;      // 防御の重み
} // namespace Score

// UI Colors
const sf::Color COLOR_BG(222, 184, 135);
const sf::Color COLOR_LINE(0, 0, 0, 200);
const sf::Color COLOR_TEXT(20, 20, 20);
} // namespace Config
