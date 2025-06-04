#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <random>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

struct ScoreEntry {
    std::string name;
    int score;
};

std::vector<ScoreEntry> loadTopScores(const std::string& filename = "bestScores.txt") {
    std::vector<ScoreEntry> scores;
    std::ifstream file(filename);
    if (!file.is_open()) return scores;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string name;
        int score;
        if (iss >> name >> score) {
            scores.push_back({name, score});
        }
    }
    file.close();

    std::sort(scores.begin(), scores.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return a.score > b.score;
    });

    if (scores.size() > 10) scores.resize(10);
    return scores;
}

void showTopScores(sf::Font& font) {
    auto scores = loadTopScores();

    sf::RenderWindow topWindow(sf::VideoMode(600, 600), "Top 10 Scores");

    sf::Text title("Top 10 Scores", font, 48);
    title.setPosition(150, 20);
    title.setFillColor(sf::Color::White);

    std::vector<sf::Text> scoreTexts;
    for (size_t i = 0; i < scores.size(); ++i) {
        sf::Text text;
        text.setFont(font);
        text.setCharacterSize(32);
        text.setFillColor(sf::Color::Yellow);
        text.setPosition(50, 100 + i * 40);
        std::ostringstream oss;
        oss << (i + 1) << ". " << scores[i].name << " - " << scores[i].score;
        text.setString(oss.str());
        scoreTexts.push_back(text);
    }

    while (topWindow.isOpen()) {
        sf::Event event;
        while (topWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                topWindow.close();
        }

        topWindow.clear(sf::Color::Black);
        topWindow.draw(title);
        for (auto& t : scoreTexts)
            topWindow.draw(t);
        topWindow.display();
    }
}

float randomX(float maxX) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0, maxX);
    return dis(gen);
}

void showGameOverScreen(sf::Font& font, const std::string& playerName, int score);

void runGame(sf::Font& font, const std::string& playerName) {
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Catcher Game");
    sf::Clock clock;

    sf::Texture basketTex;
    if (!basketTex.loadFromFile("basket.png")) {
        std::cerr << "Failed to load basket.png\n";
        return;
    }

    sf::Texture appleTex;
    if (!appleTex.loadFromFile("apple.png")) {
        std::cerr << "Failed to load apple.png\n";
        return;
    }

    sf::Sprite basket(basketTex);
    basket.setTextureRect(sf::IntRect(0, 0, 50, basketTex.getSize().y));
    basket.setScale(2.f, 2.f);
    basket.setPosition(512, 700);

    sf::Sprite apple(appleTex);
    apple.setTextureRect(sf::IntRect(0, 0, 50, appleTex.getSize().y));
    apple.setPosition(randomX(window.getSize().x - 50), 0);

    float speed = 300.f;
    float appleSpeed = 200.f;

    sf::Text timeText;
    timeText.setFont(font);
    timeText.setCharacterSize(24);
    timeText.setFillColor(sf::Color::White);
    timeText.setPosition(860, 10);

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(860, 40);

    sf::Text playerText;
    playerText.setFont(font);
    playerText.setCharacterSize(24);
    playerText.setFillColor(sf::Color::White);
    playerText.setPosition(860, 70);
    playerText.setString("Player: " + playerName);

    sf::Clock gameClock;
    int score = 0;

    while (window.isOpen()) {
        sf::Time delta = clock.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        sf::Vector2f pos = basket.getPosition();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && pos.x > 0)
            basket.move(-speed * delta.asSeconds(), 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && pos.x + basket.getGlobalBounds().width < window.getSize().x)
            basket.move(speed * delta.asSeconds(), 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && pos.y > 0)
            basket.move(0, -speed * delta.asSeconds());
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && pos.y + basket.getGlobalBounds().height < window.getSize().y)
            basket.move(0, speed * delta.asSeconds());

        apple.move(0, appleSpeed * delta.asSeconds());

        if (apple.getPosition().y > window.getSize().y) {
            apple.setPosition(randomX(window.getSize().x - 50), 0);
        }

        if (basket.getGlobalBounds().intersects(apple.getGlobalBounds())) {
            score++;
            apple.setPosition(randomX(window.getSize().x - 50), 0);
        }

        int seconds = static_cast<int>(gameClock.getElapsedTime().asSeconds());
        timeText.setString("Time: " + std::to_string(seconds));
        scoreText.setString("Points: " + std::to_string(score));

        if (seconds >= 20) {
            window.close();

            std::ofstream file("bestScores.txt", std::ios::app);
            if (file.is_open()) {
                file << playerName << " " << score << "\n";
                file.close();
            } else {
                std::cerr << "Não foi possível abrir o ficheiro para guardar o score.\n";
            }

            showGameOverScreen(font, playerName, score);
            return;
        }

        window.clear(sf::Color::Black);
        window.draw(basket);
        window.draw(apple);
        window.draw(timeText);
        window.draw(scoreText);
        window.draw(playerText);
        window.display();
    }
}

void showGameOverScreen(sf::Font& font, const std::string& playerName, int score) {
    sf::RenderWindow overWindow(sf::VideoMode(1024, 768), "Game Over");

    sf::Text gameOverText("Game Over", font, 60);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setPosition(360, 150);

    sf::Text playerText("Player: " + playerName, font, 50);
    playerText.setFillColor(sf::Color::White);
    playerText.setPosition(360, 250);

    sf::Text scoreText("Score: " + std::to_string(score), font, 40);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(440, 350);

    sf::Clock waitClock;

    while (overWindow.isOpen()) {
        sf::Event event;
        while (overWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                overWindow.close();
        }

        if (waitClock.getElapsedTime().asSeconds() >= 5) {
            overWindow.close();
        }

        overWindow.clear(sf::Color::Black);
        overWindow.draw(gameOverText);
        overWindow.draw(playerText);
        overWindow.draw(scoreText);
        overWindow.display();
    }
}

// NOVA FUNÇÃO: Pedir nome do jogador numa janela SFML
std::string askPlayerName(sf::Font& font) {
    sf::RenderWindow inputWindow(sf::VideoMode(600, 200), "Introduza o seu nome");

    sf::Text prompt("Digite seu nome e pressione ENTER:", font, 24);
    prompt.setPosition(20, 20);
    prompt.setFillColor(sf::Color::White);

    sf::Text inputText("", font, 30);
    inputText.setPosition(20, 80);
    inputText.setFillColor(sf::Color::Yellow);

    std::string playerName;

    while (inputWindow.isOpen()) {
        sf::Event event;
        while (inputWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                inputWindow.close();
                return ""; // janela fechada sem nome
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Backspace
                    if (!playerName.empty()) {
                        playerName.pop_back();
                    }
                } else if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                    if (!playerName.empty()) {
                        inputWindow.close();
                    }
                } else if (event.text.unicode < 128 && isprint(event.text.unicode)) {
                    playerName += static_cast<char>(event.text.unicode);
                }
                inputText.setString(playerName);
            }
        }

        inputWindow.clear(sf::Color::Black);
        inputWindow.draw(prompt);
        inputWindow.draw(inputText);
        inputWindow.display();
    }

    return playerName;
}

int main() {
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Failed to load arial.ttf\n";
        return 1;
    }

    while (true) {
        sf::RenderWindow menu(sf::VideoMode(1024, 768), "Main Menu");

        sf::Text title("Catch Game", font, 48);
        title.setPosition(350, 200);
        title.setFillColor(sf::Color::White);

        sf::RectangleShape playButton(sf::Vector2f(250, 60));
        playButton.setPosition(380, 320);
        playButton.setFillColor(sf::Color::Green);

        sf::RectangleShape topScoresButton(sf::Vector2f(250, 60));
        topScoresButton.setPosition(380, 420);
        topScoresButton.setFillColor(sf::Color::Blue);

        sf::Text playText("Play", font, 30);
        playText.setPosition(480, 335);
        playText.setFillColor(sf::Color::Black);

        sf::Text scoresText("Top 10 Scores", font, 25);
        scoresText.setPosition(410, 435);
        scoresText.setFillColor(sf::Color::White);

        while (menu.isOpen()) {
            sf::Event event;
            while (menu.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    menu.close();
                    return 0;
                }

                if (event.type == sf::Event::MouseButtonPressed) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(menu);
                    if (playButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        // Pede o nome aqui!
                        std::string playerName = askPlayerName(font);
                        if (!playerName.empty()) {
                            runGame(font, playerName);
                        }
                        menu.create(sf::VideoMode(1024, 768), "Main Menu");
                        ; // Recria a janela para o menu após o jogo ou cancelamento
                    }
                    else if (topScoresButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        if (!std::filesystem::exists("bestScores.txt")) {
                            std::cout << "bestScores.txt não existe!\n";
                        } else {
                            showTopScores(font);
                        }
                        menu.create(sf::VideoMode(1024, 768), "Main Menu");    // Recria o menu após fechar a janela Top Scores
                    }
                }
            }

            menu.clear(sf::Color::Black);
            menu.draw(title);
            menu.draw(playButton);
            menu.draw(topScoresButton);
            menu.draw(playText);
            menu.draw(scoresText);
            menu.display();
        }
    }

    return 0;
}

