#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <optional>
#include <filesystem>
#include <algorithm>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int TILE_SIZE = 20;
const int TOP_BAR_HEIGHT = 30; // Zone réservée en haut

// Zone de jeu (hors barre du haut)
const int GAME_AREA_HEIGHT = WINDOW_HEIGHT - TOP_BAR_HEIGHT;
const int GAME_COLS = WINDOW_WIDTH / TILE_SIZE;
const int GAME_ROWS = GAME_AREA_HEIGHT / TILE_SIZE;

// Tailles de police (modifiable)
const unsigned int TITLE_FONT_SIZE = 60;
const unsigned int START_FONT_SIZE = 40;
const unsigned int PAUSED_FONT_SIZE = 60;
const unsigned int GAMEOVER_FONT_SIZE = 60;
const unsigned int BESTSCORE_FONT_SIZE = 30;
const unsigned int SCORE_FONT_SIZE = 24;
const unsigned int PAUSEHINT_FONT_SIZE = 24;

enum class Direction { Up, Down, Left, Right };
enum class GameState { Title, Playing, Paused, GameOver };

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    sf::RenderWindow window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "Snake Game - SFML 3.0.0");
    window.setFramerateLimit(30);

    sf::Image icon;
    if (icon.loadFromFile("data/icon/snake.png")) {
        window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }

    sf::Font font;
    if (!font.openFromFile("data/fonts/Gamer.ttf"))
        throw std::runtime_error("Impossible de charger 'Gamer.ttf'");

    std::vector<int> bestScores;
    std::vector<sf::Vector2i> snake;
    std::vector<sf::Vector2i> obstacles;
    sf::Vector2i fruit;
    Direction dir = Direction::Right;
    Direction nextDir = dir;            // prochaine direction
    int score = 0;
    float moveDelay = 0.1f;
    bool gameOver = false;
    sf::Clock clock;
    GameState gameState = GameState::Title;

    auto startNewGame = [&]() {
        snake.clear();
        obstacles.clear();
        score = 0;
        dir = Direction::Right;
        nextDir = dir;
        gameOver = false;
        clock.restart();
        snake.push_back({ GAME_COLS / 2, GAME_ROWS / 2 });
        fruit.x = std::rand() % GAME_COLS;
        fruit.y = std::rand() % GAME_ROWS;
        };

    startNewGame();

    while (window.isOpen()) {
        // Gestion des événements
        while (auto eventOpt = window.pollEvent()) {
            if (!eventOpt.has_value())
                continue;
            if (eventOpt->is<sf::Event::Closed>())
                window.close();

            switch (gameState) {
            case GameState::Title:
                if (eventOpt->is<sf::Event::KeyPressed>() || eventOpt->is<sf::Event::MouseButtonPressed>()) {
                    startNewGame();
                    gameState = GameState::Playing;
                }
                break;

            case GameState::Playing:
                if (eventOpt->is<sf::Event::KeyPressed>()) {
                    const auto* keyEvent = eventOpt->getIf<sf::Event::KeyPressed>();
                    // Flèches et ZQSD
                    if ((keyEvent->scancode == sf::Keyboard::Scan::Up || keyEvent->scancode == sf::Keyboard::Scan::W)
                        && dir != Direction::Down) {
                        nextDir = Direction::Up;
                    }
                    else if ((keyEvent->scancode == sf::Keyboard::Scan::Down || keyEvent->scancode == sf::Keyboard::Scan::S)
                        && dir != Direction::Up) {
                        nextDir = Direction::Down;
                    }
                    else if ((keyEvent->scancode == sf::Keyboard::Scan::Left || keyEvent->scancode == sf::Keyboard::Scan::A)
                        && dir != Direction::Right) {
                        nextDir = Direction::Left;
                    }
                    else if ((keyEvent->scancode == sf::Keyboard::Scan::Right || keyEvent->scancode == sf::Keyboard::Scan::D)
                        && dir != Direction::Left) {
                        nextDir = Direction::Right;
                    }
                    else if (keyEvent->scancode == sf::Keyboard::Scan::Space) {
                        gameState = GameState::Paused;
                    }
                }
                break;

            case GameState::Paused:
                if (eventOpt->is<sf::Event::KeyPressed>()) {
                    const auto* keyEvent = eventOpt->getIf<sf::Event::KeyPressed>();
                    if (keyEvent->scancode == sf::Keyboard::Scan::Space)
                        gameState = GameState::Playing;
                }
                break;

            case GameState::GameOver:
                if (eventOpt->is<sf::Event::KeyPressed>() || eventOpt->is<sf::Event::MouseButtonPressed>()) {
                    startNewGame();
                    gameState = GameState::Playing;
                }
                break;
            }
        }

        // Mise à jour du jeu
        if (gameState == GameState::Playing) {
            if (clock.getElapsedTime().asSeconds() > moveDelay) {
                dir = nextDir; // Appliquer la nouvelle direction une seule fois par déplacement
                sf::Vector2i newHead = snake.front();
                switch (dir) {
                case Direction::Up:    newHead.y -= 1; break;
                case Direction::Down:  newHead.y += 1; break;
                case Direction::Left:  newHead.x -= 1; break;
                case Direction::Right: newHead.x += 1; break;
                }
                // Téléportation aux bords
                if (newHead.x < 0) newHead.x = GAME_COLS - 1;
                if (newHead.x >= GAME_COLS) newHead.x = 0;
                if (newHead.y < 0) newHead.y = GAME_ROWS - 1;
                if (newHead.y >= GAME_ROWS) newHead.y = 0;

                // Collision avec le corps ou obstacles
                for (const auto& segment : snake) {
                    if (segment == newHead) { gameOver = true; break; }
                }
                for (const auto& obs : obstacles) {
                    if (obs == newHead) { gameOver = true; break; }
                }
                if (gameOver) {
                    bestScores.push_back(score);
                    std::sort(bestScores.begin(), bestScores.end(), std::greater<int>());
                    if (bestScores.size() > 5)
                        bestScores.resize(5);
                    gameState = GameState::GameOver;
                    continue;
                }

                snake.insert(snake.begin(), newHead);
                // Gestion de la récolte de fruit
                if (newHead == fruit) {
                    score += 10;
                    bool validPosition = false;
                    while (!validPosition) {
                        fruit.x = std::rand() % GAME_COLS;
                        fruit.y = std::rand() % GAME_ROWS;
                        validPosition = true;
                        for (const auto& segment : snake) {
                            if (segment == fruit) { validPosition = false; break; }
                        }
                        for (const auto& obs : obstacles) {
                            if (obs == fruit) { validPosition = false; break; }
                        }
                    }
                    // Ajout de nouveaux obstacles
                    for (int i = 0; i < 2; i++) {
                        sf::Vector2i obs;
                        bool validObs = false;
                        while (!validObs) {
                            obs.x = std::rand() % GAME_COLS;
                            obs.y = std::rand() % GAME_ROWS;
                            validObs = true;
                            if (obs == fruit) validObs = false;
                            for (const auto& segment : snake) {
                                if (segment == obs) { validObs = false; break; }
                            }
                            for (const auto& existingObs : obstacles) {
                                if (existingObs == obs) { validObs = false; break; }
                            }
                        }
                        obstacles.push_back(obs);
                    }
                }
                else {
                    snake.pop_back();
                }
                clock.restart();
            }
        }

        // Dessin
        window.clear(sf::Color(150, 180, 120));

        // Barre du haut
        {
            sf::Text scoreText(font);
            scoreText.setString("Score: " + std::to_string(score));
            scoreText.setCharacterSize(SCORE_FONT_SIZE);
            scoreText.setFillColor(sf::Color::White);
            scoreText.setPosition({ 5.f, 5.f });

            sf::Text pauseHint(font);
            pauseHint.setCharacterSize(PAUSEHINT_FONT_SIZE);
            pauseHint.setFillColor(sf::Color::White);
            if (gameState == GameState::Paused)
                pauseHint.setString("Appuyez sur ESPACE pour reprendre");
            else
                pauseHint.setString("Appuyez sur ESPACE pour mettre en pause");
            float textWidth = pauseHint.getLocalBounds().size.x;
            pauseHint.setPosition({ WINDOW_WIDTH - textWidth - 10.f, 5.f });

            sf::RectangleShape barBG({ static_cast<float>(WINDOW_WIDTH), 30.f });
            barBG.setFillColor(sf::Color(50, 50, 50, 200));
            barBG.setPosition({ 0.f, 0.f });
            window.draw(barBG);
            window.draw(scoreText);
            window.draw(pauseHint);
        }

        // Écrans d'état
        if (gameState == GameState::Title) {
            sf::Text titleText(font, "Bienvenue dans Snake Game SFML 3.0.0\nCodée par Zakariyae Allali", TITLE_FONT_SIZE);
            titleText.setFillColor(sf::Color::White);
            sf::FloatRect bounds = titleText.getLocalBounds();
            float xPos = WINDOW_WIDTH / 2.f - bounds.size.x / 2.f;
            float yPos = WINDOW_HEIGHT / 2.f - bounds.size.y / 2.f - 50.f;
            titleText.setPosition({ xPos, yPos });
            window.draw(titleText);

            sf::Text startText(font, "Appuyez sur une touche ou cliquez pour démarrer", START_FONT_SIZE);
            startText.setFillColor(sf::Color::Yellow);
            float sxPos = WINDOW_WIDTH / 2.f - startText.getLocalBounds().size.x / 2.f;
            float syPos = WINDOW_HEIGHT / 2.f + 50.f;
            startText.setPosition({ sxPos, syPos });
            window.draw(startText);

            // Indication des touches ZQSD - ARROWS
            sf::Text controlsText(font, "Touches: Z (haut), Q (gauche), S (bas), D (droite)", PAUSEHINT_FONT_SIZE);
            controlsText.setFillColor(sf::Color::White);
            float cxPos = WINDOW_WIDTH / 2.f - controlsText.getLocalBounds().size.x / 2.f;
            controlsText.setPosition({ cxPos, syPos + 50.f });
            window.draw(controlsText);
            sf::Text arrowHint(font, "Vous pouvez aussi utiliser les flèches (haut, bas, gauche, droite)", PAUSEHINT_FONT_SIZE);
            arrowHint.setFillColor(sf::Color::White);
            float axPos = WINDOW_WIDTH / 2.f - arrowHint.getLocalBounds().size.x / 2.f;
            arrowHint.setPosition({ axPos, syPos + 85.f });
            window.draw(arrowHint);
        }
        else if (gameState == GameState::Playing || gameState == GameState::Paused) {
            // Dessin du serpent
            for (const auto& segment : snake) {
                sf::RectangleShape rect({ static_cast<float>(TILE_SIZE - 2), static_cast<float>(TILE_SIZE - 2) });
                rect.setFillColor(sf::Color::Blue);
                rect.setPosition({ static_cast<float>(segment.x * TILE_SIZE), static_cast<float>(segment.y * TILE_SIZE + TOP_BAR_HEIGHT) });
                window.draw(rect);
            }
            // Fruit
            sf::RectangleShape fruitRect({ static_cast<float>(TILE_SIZE - 2), static_cast<float>(TILE_SIZE - 2) });
            fruitRect.setFillColor(sf::Color::Red);
            fruitRect.setPosition({ static_cast<float>(fruit.x * TILE_SIZE), static_cast<float>(fruit.y * TILE_SIZE + TOP_BAR_HEIGHT) });
            window.draw(fruitRect);
            // Obstacles
            for (const auto& obs : obstacles) {
                sf::RectangleShape obsRect({ static_cast<float>(TILE_SIZE - 2), static_cast<float>(TILE_SIZE - 2) });
                obsRect.setFillColor(sf::Color::Green);
                obsRect.setPosition({ static_cast<float>(obs.x * TILE_SIZE), static_cast<float>(obs.y * TILE_SIZE + TOP_BAR_HEIGHT) });
                window.draw(obsRect);
            }
            // Texte PAUSE
            if (gameState == GameState::Paused) {
                sf::Text pausedText(font, "PAUSE", PAUSED_FONT_SIZE);
                pausedText.setFillColor(sf::Color::White);
                float px = WINDOW_WIDTH / 2.f - pausedText.getLocalBounds().size.x / 2.f;
                float py = WINDOW_HEIGHT / 2.f - pausedText.getLocalBounds().size.y / 2.f;
                pausedText.setPosition({ px, py });
                window.draw(pausedText);
            }
        }
        else if (gameState == GameState::GameOver) {
            sf::Text gameOverText(font, "GAME OVER", GAMEOVER_FONT_SIZE);
            gameOverText.setFillColor(sf::Color::White);
            float gameoverx = WINDOW_WIDTH / 2.f - gameOverText.getLocalBounds().size.x / 2.f;
            gameOverText.setPosition({ gameoverx, WINDOW_HEIGHT / 2.f - 100.f });
            window.draw(gameOverText);

            sf::Text bestScoreTitle(font, "TOP 5 SCORES:", BESTSCORE_FONT_SIZE);
            bestScoreTitle.setFillColor(sf::Color::Yellow);
            float bestscoretx = WINDOW_WIDTH / 2.f - bestScoreTitle.getLocalBounds().size.x / 2.f;
            bestScoreTitle.setPosition({ bestscoretx, WINDOW_HEIGHT / 2.f });
            window.draw(bestScoreTitle);
            std::string bestScoresStr;
            for (size_t i = 0; i < bestScores.size(); i++) {
                bestScoresStr += std::to_string(i + 1) + ". " + std::to_string(bestScores[i]) + "\n";
            }
            sf::Text bestScoresText(font, bestScoresStr, BESTSCORE_FONT_SIZE);
            bestScoresText.setFillColor(sf::Color::Yellow);
            float bestscorex = WINDOW_WIDTH / 2.f - bestScoresText.getLocalBounds().size.x / 2.f;
            bestScoresText.setPosition({ bestscorex, WINDOW_HEIGHT / 2.f + 25.f });
            window.draw(bestScoresText);

            sf::Text restartText(font, "Appuyez sur une touche ou cliquez pour redémarrer", PAUSEHINT_FONT_SIZE);
            restartText.setFillColor(sf::Color::White);
            float restx = WINDOW_WIDTH / 2.f - restartText.getLocalBounds().size.x / 2.f;
            restartText.setPosition({ restx, WINDOW_HEIGHT - 100.f });
            window.draw(restartText);
        }

        window.display();
    }
    return 0;
}
