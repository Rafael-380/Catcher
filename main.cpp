#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <random>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>

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
    title.setPosition(50, 20);
    title.setFillColor(sf::Color::White);

    sf::Text continueText("Press Enter to return to menu", font, 30);
    continueText.setPosition(50, 650);
    continueText.setFillColor(sf::Color::Yellow);

    std::vector<sf::Text> scoreTexts;
    for (size_t i = 0; i < scores.size(); ++i) {
        sf::Text text;
        text.setFont(font);
        text.setCharacterSize(32);
        text.setFillColor(sf::Color::White);
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
    bool paused = false;
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

    sf::RectangleShape topBar;
    topBar.setSize(sf::Vector2f(window.getSize().x, 30));
    topBar.setPosition(0, 0);
    topBar.setFillColor(sf::Color::Black);

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
    basket.setScale(2.5f, 2.5f);
    basket.setPosition(512, 650);

    std::vector<sf::Sprite> apples;
    sf::Sprite firstApple(appleTex);
    firstApple.setTextureRect(sf::IntRect(
        10,
        9,
        50 - 20,
        appleTex.getSize().y - (9+2)            //cropping 9px on top plus 2 at the bottom
        ));
    firstApple.setPosition(randomX(window.getSize().x - 50), 50);
    apples.push_back(firstApple);

    float speed = 300.f;
    float baseAppleSpeed = 200.f;

    // Dimensões da janela
    float windowWidth = 1024;

    // Texto: Player (esquerda)
    sf::Text playerText;
    playerText.setFont(font);
    playerText.setCharacterSize(18);
    playerText.setFillColor(sf::Color::White);
    playerText.setString("Player: " + playerName);
    playerText.setPosition(10, 5);  // Margem esquerda

    // Texto: Tempo (centro)
    sf::Text timeText;
    timeText.setFont(font);
    timeText.setCharacterSize(18);
    timeText.setFillColor(sf::Color::White);
    timeText.setString("Time: 00:00");  // Atualiza este valor conforme o tempo real
    float timeTextWidth = timeText.getLocalBounds().width;
    timeText.setPosition((windowWidth - timeTextWidth) / 2, 5);

    // Texto: Score (direita)
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(18);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setString("Score: 0");  // Atualiza este valor conforme o score real
    float scoreTextWidth = scoreText.getLocalBounds().width;
    scoreText.setPosition(windowWidth - scoreTextWidth - 30, 5);  // Margem direita

    sf::Text pauseText("PAUSED\nPress SPACE to resume", font, 48);
    pauseText.setFillColor(sf::Color::Yellow);
    pauseText.setOutlineColor(sf::Color::Black);
    pauseText.setOutlineThickness(2);
    pauseText.setPosition(250, 300); // Ajusta conforme necessário


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
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                paused = !paused;       //If you are paused, unpouses. If you are playing, pauses
            }
        }

        if (!paused) {
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
                newApple.setPosition(randomX(window.getSize().x - 50), 50);
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
                    apple.setPosition(randomX(window.getSize().x - 50), 50);
                }

                if (basket.getGlobalBounds().intersects(apple.getGlobalBounds())) {
                    score++;
                    apple.setPosition(randomX(window.getSize().x - 50), 50);
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
                    std::cerr << "It wasnt possible to store the score.\n";
                }

                showGameOverScreen(font, playerName, score);
                return;
            }
        }

        window.clear();
        window.draw(background);
        window.draw(topBar);                  // Desenhar a barra preta
        window.draw(basket);
        for (auto& apple : apples){
            window.draw(apple);
        }
        window.draw(timeText);
        window.draw(scoreText);
        window.draw(playerText);
        if (paused) {
            window.draw(pauseText);
        }
        window.display();
    }
}


void showGameOverScreen(sf::Font& font, const std::string& playerName, int score) {
    // Determinar o maior score existente
    int maxScore = 0;
    std::ifstream inFile("bestScores.txt");
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;

            while (iss >> token)
                tokens.push_back(token);

            if (tokens.size() < 2) continue;

            try {
                int pastScore = std::stoi(tokens.back());
                if (pastScore > maxScore)
                    maxScore = pastScore;
            } catch (...) {
                continue; // ignora linhas mal formatadas
            }
        }
        inFile.close();
    }

    bool isNewRecord = score > maxScore;

    // Criar janela de Game Over
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

    sf::Text recordText;
    if (isNewRecord) {
        recordText.setString("NEW RECORD!");
        recordText.setFillColor(sf::Color::Green);
    } else {
        recordText.setString("High score: " + std::to_string(maxScore));
        recordText.setFillColor(sf::Color(200, 200, 200));
    }
    recordText.setFont(font);
    recordText.setCharacterSize(32);
    recordText.setPosition(390, 420);

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
        overWindow.draw(recordText);
        overWindow.draw(continueText);
        overWindow.display();
    }
}

// Pedir nome do jogador numa janela SFML
std::string askPlayerName(sf::Font& font) {
    sf::RenderWindow inputWindow(sf::VideoMode(1024, 768), "Catcher - Name");

    sf::Text prompt("Write your name (max 20 characters) and press ENTER:", font, 24);
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
                } else if ((event.text.unicode == '\r' || event.text.unicode == '\n') && !playerName.empty()) {
                    inputWindow.close();
                } else if (event.text.unicode < 128 && isprint(event.text.unicode)) {
                    if (playerName.size() < 20) { // Limite de 20 caracteres
                        playerName += static_cast<char>(event.text.unicode);
                    }
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
        title.setFillColor(sf::Color::White);
        // Calcula a posição X centralizada
        title.setPosition(512 - title.getLocalBounds().width / 2, 100);

        const float windowWidth = 1024;
        const float buttonWidth = 250;
        const float buttonHeight = 60;
        const float buttonMiddleX = (windowWidth - buttonWidth) / 2; // 768/2

        sf::RectangleShape playButton(sf::Vector2f(buttonWidth, buttonHeight));
        playButton.setPosition(buttonMiddleX, 220);
        playButton.setFillColor(sf::Color::Green);

        sf::RectangleShape topScoresButton(sf::Vector2f(buttonWidth, buttonHeight));
        topScoresButton.setPosition(buttonMiddleX, 320);
        topScoresButton.setFillColor(sf::Color::Blue);

        sf::RectangleShape quitButton(sf::Vector2f(buttonWidth, buttonHeight));
        quitButton.setPosition(buttonMiddleX, 420);
        quitButton.setFillColor(sf::Color::Red);

        sf::RectangleShape creditsButton(sf::Vector2f(buttonWidth, buttonHeight));
        creditsButton.setPosition(buttonMiddleX, 520);
        creditsButton.setFillColor(sf::Color::Yellow);

        sf::RectangleShape rulesButton(sf::Vector2f(buttonWidth, buttonHeight));
        rulesButton.setPosition(buttonMiddleX, 620);
        rulesButton.setFillColor(sf::Color::Cyan);

        // Agora os textos centralizados horizontalmente sobre cada botão
        sf::Text playText("Play", font, 30);
        playText.setFillColor(sf::Color::Black);
        playText.setPosition(
            buttonMiddleX + (buttonWidth - playText.getLocalBounds().width) / 2, 235);

        sf::Text scoresText("Top 10 Scores", font, 25);
        scoresText.setFillColor(sf::Color::White);
        scoresText.setPosition(
            buttonMiddleX + (buttonWidth - scoresText.getLocalBounds().width) / 2, 335);

        sf::Text quitText("Leave Game", font, 25);
        quitText.setFillColor(sf::Color::White);
        quitText.setPosition(
            buttonMiddleX + (buttonWidth - quitText.getLocalBounds().width) / 2, 435);

        sf::Text creditsText("Credits", font, 25);
        creditsText.setFillColor(sf::Color::Black);
        creditsText.setPosition(
            buttonMiddleX + (buttonWidth - creditsText.getLocalBounds().width) / 2, 535);

        sf::Text rulesText("Rules", font, 25);
        rulesText.setFillColor(sf::Color::Black);
        rulesText.setPosition(
            buttonMiddleX + (buttonWidth - rulesText.getLocalBounds().width) / 2, 635);

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
                        sf::RenderWindow credits(sf::VideoMode(1024, 768), "Catcher - Credits");

                        sf::Text title("Catch Game", font, 48);
                        title.setFillColor(sf::Color::White);
                        // Calcula a posição X centralizada
                        title.setPosition(512 - title.getLocalBounds().width / 2, 100);


                        sf::Text text("Game developed by Rafael Louro  with the help of AI tools\n"
                                      "Basket and apple designed by Rafael Louro in https://www.pixilart.com \n"
                                      "The other textures come from https://www.freepik.com", font, 24);
                        text.setFillColor(sf::Color::White);
                        text.setPosition(50, 50);

                        sf::Text backText("Press ENTER to return to the menu", font, 24);
                        backText.setFillColor(sf::Color::Yellow);
                        backText.setPosition(50, 200);





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
                    else if (rulesButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        // Abre a janela de regras
                        sf::RenderWindow rules(sf::VideoMode(1024, 768), "Catcher - Rules");
                        sf::Text text("Rules: \n"
                                      " - Catch the fruits with your basket! \n"
                                      " - Move the basket using the arrows \n"
                                      " - You have 1 minute to get has many points as possible \n"
                                      " - You can pause the game using SPACE \n"
                                      " - Good luck :)", font, 24);
                        text.setFillColor(sf::Color::White);
                        text.setPosition(50, 50);

                        sf::Text backText("Press ENTER to return to the menu", font, 24);
                        backText.setFillColor(sf::Color::Yellow);
                        backText.setPosition(50, 300);

                        while (rules.isOpen()) {
                            sf::Event e;
                            while (rules.pollEvent(e)) {
                                if (e.type == sf::Event::Closed)
                                    rules.close();
                                if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter)
                                    rules.close();
                            }
                            rules.clear(sf::Color::Black);
                            rules.draw(text);
                            rules.draw(backText);
                            rules.display();
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
            menu.draw(rulesButton);
            menu.draw(rulesText);
            menu.display();
        }
    }

    return 0;
}

