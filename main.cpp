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

    sf::RenderWindow topWindow(sf::VideoMode(1024, 768), "Catcher - Top 10");

    sf::Text title("Top 10 Scores", font, 48);
    title.setPosition(150, 20);
    title.setFillColor(sf::Color::White);

    sf::Text continueText("Press Enter to return to menu", font, 30);
    continueText.setPosition(250, 650);
    continueText.setFillColor(sf::Color::Yellow);

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

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                topWindow.close();
        }

        topWindow.clear(sf::Color::Black);
        topWindow.draw(title);
        for (auto& t : scoreTexts)
            topWindow.draw(t);
        topWindow.draw(continueText);
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
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Catcher - Game");
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

    sf::Texture backgroundTex;
    if (!backgroundTex.loadFromFile("background.png")) {
        std::cerr << "Failed to load background.png\n";
        return;
    }

    sf::Sprite background(backgroundTex);
    background.setScale(
        static_cast<float>(window.getSize().x) / backgroundTex.getSize().x,
        static_cast<float>(window.getSize().y) / backgroundTex.getSize().y
    );

    sf::Sprite basket(basketTex);
    //basket.setTextureRect(sf::IntRect(0, 0, 50, basketTex.getSize().y));
    basket.setTextureRect(sf::IntRect(
        4,                                // left crop (start 4px in)
        10,                                // top crop (start 8px down)
        50 - 8,                            // width after removing 4px from each side
        basketTex.getSize().y - 20        // height after removing 8px from top and bottom
        ));
    basket.setScale(2.f, 2.f);
    basket.setPosition(512, 650);

    std::vector<sf::Sprite> apples;
    sf::Sprite firstApple(appleTex);
    firstApple.setTextureRect(sf::IntRect(
        10,
        9,
        50 - 20,
        appleTex.getSize().y - (9+2)            //cropping 9px on top plus 2 at the bottom
        ));
    firstApple.setPosition(randomX(window.getSize().x - 50), 0);
    apples.push_back(firstApple);

    float speed = 300.f;
    float baseAppleSpeed = 200.f;

    sf::Text timeText;
    timeText.setFont(font);
    timeText.setCharacterSize(24);
    timeText.setFillColor(sf::Color::Black);
    timeText.setPosition(860, 10);

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::Black);
    scoreText.setPosition(860, 40);

    sf::Text playerText;
    playerText.setFont(font);
    playerText.setCharacterSize(24);
    playerText.setFillColor(sf::Color::Black);
    playerText.setPosition(860, 70);
    playerText.setString("Player: " + playerName);

    sf::Clock gameClock;
    sf::Clock appleClock;
    int score = 0;
    int applesToSpawn = 1;

    while (window.isOpen()) {
        sf::Time delta = clock.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Movimento do cesto
        sf::Vector2f pos = basket.getPosition();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && pos.x > 0)
            basket.move(-speed * delta.asSeconds(), 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && pos.x + basket.getGlobalBounds().width < window.getSize().x)
            basket.move(speed * delta.asSeconds(), 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && pos.y > 0)
            basket.move(0, -speed * delta.asSeconds());
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && pos.y + basket.getGlobalBounds().height < window.getSize().y)
            basket.move(0, speed * delta.asSeconds());

        // Adiciona nova maçã a cada 10 segundos (até 6 no total)
        if (appleClock.getElapsedTime().asSeconds() >= 10.f && applesToSpawn < 6) {
            sf::Sprite newApple(appleTex);
            //newApple.setTextureRect(sf::IntRect(0, 0, 50, appleTex.getSize().y));
            newApple.setTextureRect(sf::IntRect(
                10,                       // crop 10 px from the left
                9,                        // crop 9 px from the top
                50 - 20,                  // crop 10 px from right, width becomes 30
                appleTex.getSize().y - 11 // crop 2 px from bottom, total height adjusted
                ));
            newApple.setPosition(randomX(window.getSize().x - 50), 0);
            apples.push_back(newApple);
            applesToSpawn++;
            appleClock.restart();
        }

        // Aumenta progressivamente a velocidade das maçãs
        float elapsedSeconds = gameClock.getElapsedTime().asSeconds();
        float currentAppleSpeed = baseAppleSpeed + elapsedSeconds * 4.0f;

        // Movimento e colisão das maçãs
        for (auto& apple : apples) {
            apple.move(0, currentAppleSpeed * delta.asSeconds());

            if (apple.getPosition().y > window.getSize().y) {
                apple.setPosition(randomX(window.getSize().x - 50), 0);
            }

            if (basket.getGlobalBounds().intersects(apple.getGlobalBounds())) {
                score++;
                apple.setPosition(randomX(window.getSize().x - 50), 0);
            }
        }

        int seconds = static_cast<int>(elapsedSeconds);
        timeText.setString("Time: " + std::to_string(seconds));
        scoreText.setString("Points: " + std::to_string(score));

        if (seconds >= 60) {
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

        window.clear();
        window.draw(background);
        window.draw(basket);
        for (auto& apple : apples)
            window.draw(apple);
        window.draw(timeText);
        window.draw(scoreText);
        window.draw(playerText);
        window.display();
    }
}

void showGameOverScreen(sf::Font& font, const std::string& playerName, int score) {
    sf::RenderWindow overWindow(sf::VideoMode(1024, 768), "Catcher - Game Over");

    sf::Text gameOverText("Game Over", font, 60);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setPosition(360, 150);

    sf::Text playerText("Player: " + playerName, font, 50);
    playerText.setFillColor(sf::Color::White);
    playerText.setPosition(360, 250);

    sf::Text scoreText("Score: " + std::to_string(score), font, 40);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(440, 350);

    sf::Text continueText("Press Enter to return to menu", font, 30);
    continueText.setFillColor(sf::Color::Yellow);
    continueText.setPosition(320, 500);

    while (overWindow.isOpen()) {
        sf::Event event;
        while (overWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                overWindow.close();

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                overWindow.close();
        }

        overWindow.clear(sf::Color::Black);
        overWindow.draw(gameOverText);
        overWindow.draw(playerText);
        overWindow.draw(scoreText);
        overWindow.draw(continueText);
        overWindow.display();
    }
}


// Pedir nome do jogador numa janela SFML
std::string askPlayerName(sf::Font& font) {
    sf::RenderWindow inputWindow(sf::VideoMode(1024, 768), "Catcher - Name");

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
        sf::RenderWindow menu(sf::VideoMode(1024, 768), "Catcher - Main Menu");

        sf::Text title("Catch Game", font, 48);
        title.setPosition(350, 200);
        title.setFillColor(sf::Color::White);

        sf::RectangleShape playButton(sf::Vector2f(250, 60));
        playButton.setPosition(380, 320);
        playButton.setFillColor(sf::Color::Green);

        sf::RectangleShape topScoresButton(sf::Vector2f(250, 60));
        topScoresButton.setPosition(380, 420);
        topScoresButton.setFillColor(sf::Color::Blue);

        sf::RectangleShape quitButton(sf::Vector2f(250, 60));
        quitButton.setPosition(380, 520);
        quitButton.setFillColor(sf::Color::Red);

        sf::RectangleShape creditsButton(sf::Vector2f(250, 60));
        creditsButton.setPosition(380, 620);
        creditsButton.setFillColor(sf::Color::Yellow);

        sf::Text playText("Play", font, 30);
        playText.setPosition(480, 335);
        playText.setFillColor(sf::Color::Black);

        sf::Text scoresText("Top 10 Scores", font, 25);
        scoresText.setPosition(410, 435);
        scoresText.setFillColor(sf::Color::White);

        sf::Text quitText("Leave Game", font, 25);
        quitText.setPosition(430, 535);
        quitText.setFillColor(sf::Color::White);

        sf::Text creditsText("Credits", font, 25);
        creditsText.setPosition(450, 635);
        creditsText.setFillColor(sf::Color::Black);

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
                    }
                    else if (topScoresButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        if (!std::filesystem::exists("bestScores.txt")) {
                            std::cout << "bestScores.txt nao existe!\n";
                        } else {
                            showTopScores(font);
                        }
                        menu.create(sf::VideoMode(1024, 768), "Main Menu"); // Recria o menu após fechar a janela Top Scores
                    }
                    else if (quitButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        return 0; // Termina o programa ao clicar "Sair do Jogo"
                    }
                    else if (creditsButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        // Abre a janela de créditos
                        sf::RenderWindow credits(sf::VideoMode(1024, 768), "Créditos");
                        sf::Text text("Game developed by Rafael Louro \n"
                                      "Basket and apple designed by Rafael Louro in https://www.pixilart.com \n"
                                      "The other textures come from https://www.freepik.com", font, 24);
                        text.setFillColor(sf::Color::White);
                        text.setPosition(100, 300);

                        sf::Text backText("Pressione Enter para voltar", font, 22);
                        backText.setFillColor(sf::Color::Yellow);
                        backText.setPosition(320, 600);

                        while (credits.isOpen()) {
                            sf::Event e;
                            while (credits.pollEvent(e)) {
                                if (e.type == sf::Event::Closed)
                                    credits.close();
                                if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter)
                                    credits.close();
                            }
                            credits.clear(sf::Color::Black);
                            credits.draw(text);
                            credits.draw(backText);
                            credits.display();
                        }

                        menu.create(sf::VideoMode(1024, 768), "Main Menu");
                    }
                }
            }

            menu.clear(sf::Color::Black);
            menu.draw(title);
            menu.draw(playButton);
            menu.draw(topScoresButton);
            menu.draw(playText);
            menu.draw(scoresText);
            menu.draw(quitButton);
            menu.draw(quitText);
            menu.draw(creditsButton);
            menu.draw(creditsText);
            menu.display();
        }
    }

    return 0;
}

