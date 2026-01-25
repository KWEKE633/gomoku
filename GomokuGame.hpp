#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Board.hpp"
#include "AI.hpp"
#include "Config.hpp"
#include "Types.hpp"

class GomokuGame
{
private:
	sf::RenderWindow window;
	Board board;
	AI ai;

	sf::Font font;
	sf::Text statusText;
	sf::Text guideText;
	sf::Text timerText;

	GameMode mode;
	Player userColor;
	bool gameOver;
	Player winner;

	std::vector<std::pair<int, int>> moveHistory;
	int replayIndex;
	bool isReplayMode;

public:
	GomokuGame();
	void run();

private:
	void loadFont();
	void initText();
	void processEvents();
	void handleMouseClick(int mx, int my);
	void doMove(int y, int x);
	void update();
	void render();
	void drawStone(int y, int x, sf::Color c);
	void updateStatusText();
	void resetGame();

	// リプレイ関連
	void startReplay();
	void stopReplay();
	void replayPrev();
	void replayNext();
	void reconstructBoard(int untilIndex);
};
