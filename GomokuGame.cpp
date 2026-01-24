#include "GomokuGame.hpp"
#include <iostream>
#include <string> // std::to_string用

GomokuGame::GomokuGame() 
    : window(sf::VideoMode(Config::WINDOW_W, Config::WINDOW_H), "42 Gomoku AI - High Defense"),
      statusText(),
      guideText(),
      timerText(),
      mode(GameMode::HumanVsAI),
      userColor(BLACK),
      gameOver(false),
      winner(NONE),
      replayIndex(-1),
      isReplayMode(false)
{
    window.setFramerateLimit(60);
    loadFont();
    initText();
}

void GomokuGame::run() {
    while (window.isOpen()) {
        processEvents();
        if (!isReplayMode) update();
        render();
    }
}

void GomokuGame::loadFont() {
    const char* fonts[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "Arial.ttf"
    };
    for (int i = 0; i < 4; ++i) {
        if (font.loadFromFile(fonts[i])) return;
    }
    std::cerr << "Warning: No font found." << std::endl;
}

void GomokuGame::initText() {
    statusText.setFont(font);
    statusText.setCharacterSize(24);
    statusText.setFillColor(Config::COLOR_TEXT);
    statusText.setPosition(20.f, (float)Config::WINDOW_H - 90.f);

    guideText.setFont(font);
    guideText.setCharacterSize(16);
    guideText.setFillColor(sf::Color(50, 50, 50));
    guideText.setPosition(20.f, (float)Config::WINDOW_H - 50.f);
    guideText.setString("Keys: [1]PvAI [2]PvP [3]SwapColor [L]Replay [R]Reset [ESC]Quit");

    timerText.setFont(font);
    timerText.setCharacterSize(20);
    timerText.setFillColor(sf::Color::Red);
    timerText.setPosition((float)Config::WINDOW_W - 150.f, (float)Config::WINDOW_H - 90.f);
}

void GomokuGame::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
        
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) window.close();
            if (event.key.code == sf::Keyboard::R) resetGame();
            
            if (!isReplayMode) {
                if (event.key.code == sf::Keyboard::Num1) { mode = GameMode::HumanVsAI; resetGame(); }
                if (event.key.code == sf::Keyboard::Num2) { mode = GameMode::HumanVsHuman; resetGame(); }
                if (event.key.code == sf::Keyboard::Num3) { userColor = (userColor == BLACK ? WHITE : BLACK); resetGame(); }
                if (event.key.code == sf::Keyboard::L) startReplay();
            } else {
                if (event.key.code == sf::Keyboard::L) stopReplay();
                if (event.key.code == sf::Keyboard::Right) replayNext();
                if (event.key.code == sf::Keyboard::Left) replayPrev();
            }
        }

        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                if (!isReplayMode && !gameOver) {
                    if (mode == GameMode::HumanVsAI && board.currentTurn != userColor) return;
                    handleMouseClick(event.mouseButton.x, event.mouseButton.y);
                }
            }
        }
    }
}

void GomokuGame::handleMouseClick(int mx, int my) {
    if (mx < Config::OFFSET || my < Config::OFFSET) return;
    int x = (mx - Config::OFFSET + Config::CELL_SIZE / 2) / Config::CELL_SIZE;
    int y = (my - Config::OFFSET + Config::CELL_SIZE / 2) / Config::CELL_SIZE;

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

void GomokuGame::doMove(int y, int x) {
    moveHistory.push_back(std::make_pair(y, x));
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

void GomokuGame::update() {
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

void GomokuGame::render() {
    window.clear(Config::COLOR_BG);

    for (int i = 0; i < Config::BOARD_SIZE; ++i) {
        sf::RectangleShape hline(sf::Vector2f(Config::CELL_SIZE * (Config::BOARD_SIZE - 1), 2));
        hline.setFillColor(Config::COLOR_LINE);
        hline.setPosition((float)Config::OFFSET, (float)(Config::OFFSET + i * Config::CELL_SIZE));
        window.draw(hline);

        sf::RectangleShape vline(sf::Vector2f(2, Config::CELL_SIZE * (Config::BOARD_SIZE - 1)));
        vline.setFillColor(Config::COLOR_LINE);
        vline.setPosition((float)(Config::OFFSET + i * Config::CELL_SIZE), (float)Config::OFFSET);
        window.draw(vline);
    }

    for (int y = 0; y < Config::BOARD_SIZE; ++y) {
        for (int x = 0; x < Config::BOARD_SIZE; ++x) {
            Player p = board.get(y, x);
            if (p != NONE) {
                drawStone(y, x, p == BLACK ? sf::Color::Black : sf::Color::White);
            }
        }
    }

    if (board.lastMove.y != -1) {
        sf::CircleShape mark(4);
        mark.setOrigin(4, 4);
        mark.setFillColor(sf::Color::Red);
        mark.setPosition((float)(Config::OFFSET + board.lastMove.x * Config::CELL_SIZE), (float)(Config::OFFSET + board.lastMove.y * Config::CELL_SIZE));
        window.draw(mark);
    }

    window.draw(statusText);
    window.draw(guideText);
    window.draw(timerText);

    sf::Text capsInfo;
    capsInfo.setFont(font);
    capsInfo.setCharacterSize(18);
    capsInfo.setFillColor(Config::COLOR_TEXT);
    capsInfo.setPosition((float)Config::WINDOW_W - 200.f, 50.f);
    capsInfo.setString("Captures:\nBlack: " + std::to_string(board.captures[BLACK]) + "/10\nWhite: " + std::to_string(board.captures[WHITE]) + "/10");
    window.draw(capsInfo);

    window.display();
}

void GomokuGame::drawStone(int y, int x, sf::Color c) {
    sf::CircleShape stone(Config::CELL_SIZE / 2 - 2);
    stone.setOrigin((float)(Config::CELL_SIZE / 2 - 2), (float)(Config::CELL_SIZE / 2 - 2));
    stone.setFillColor(c);
    stone.setPosition((float)(Config::OFFSET + x * Config::CELL_SIZE), (float)(Config::OFFSET + y * Config::CELL_SIZE));
    stone.setOutlineThickness(1);
    stone.setOutlineColor(sf::Color(50, 50, 50));
    window.draw(stone);
}

void GomokuGame::updateStatusText() {
    std::string turnStr = (board.currentTurn == BLACK) ? "Black" : "White";
    statusText.setString("Turn: " + turnStr + (isReplayMode ? " [REPLAY]" : ""));
}

void GomokuGame::resetGame() {
    board.reset();
    moveHistory.clear();
    gameOver = false;
    winner = NONE;
    isReplayMode = false;
    updateStatusText();
    timerText.setString("");
}

void GomokuGame::startReplay() {
    isReplayMode = true;
    replayIndex = (int)moveHistory.size();
    updateStatusText();
}

void GomokuGame::stopReplay() {
    isReplayMode = false;
    while (replayIndex < (int)moveHistory.size()) replayNext();
    updateStatusText();
}

void GomokuGame::replayPrev() {
    if (replayIndex > 0) {
        replayIndex--;
        reconstructBoard(replayIndex);
    }
}

void GomokuGame::replayNext() {
    if (replayIndex < (int)moveHistory.size()) {
        replayIndex++;
        reconstructBoard(replayIndex);
    }
}

void GomokuGame::reconstructBoard(int untilIndex) {
    board.reset();
    for (int i = 0; i < untilIndex; ++i) {
        board.makeMove(moveHistory[i].first, moveHistory[i].second);
    }
}
