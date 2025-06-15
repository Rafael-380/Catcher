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

    //Sorting the scores to show the best of them
    std::sort(scores.begin(), scores.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return a.score > b.score;
    });

    if (scores.size() > 10) scores.resize(10);
    return scores;
}


void showTopScores(sf::Font& font, const sf::Texture& backgroundTexture) {
    const std::string filename = "bestScores.txt";

    //Verifies if the file exists, if not creates it
    if (!std::filesystem::exists(filename)) {
        std::ofstream createFile(filename);
        if (!createFile) {
            std::cerr << "Erro ao criar " << filename << "\n";
            return;
        }
    }

    auto scores = loadTopScores();

    //Texts in the window
    sf::Sprite background(backgroundTexture);
    background.setColor(sf::Color(255, 255, 255, 128));
    sf::Vector2u textureSize = backgroundTexture.getSize();
    background.setScale(1024.f / textureSize.x, 768.f / textureSize.y);

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

    //Keep the window open until Enter is pressed and draws the texts
    while (topWindow.isOpen()) {
        sf::Event event;
        while (topWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                topWindow.close();

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                topWindow.close();
        }

        topWindow.clear();
        topWindow.draw(background);
        topWindow.draw(title);
        for (auto& t : scoreTexts)
            topWindow.draw(t);
        topWindow.draw(continueText);
        topWindow.display();
    }
}

//For spawning random apples
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

    // Load of textures
    sf::Texture basketTex, appleTex, backgroundTex, treeTex;
    if (!basketTex.loadFromFile("basket.png") ||
        !appleTex.loadFromFile("apple.png") ||
        !backgroundTex.loadFromFile("background.png") ||
        !treeTex.loadFromFile("tree.png")) {
        std::cerr << "Erro ao carregar imagens.\n";
        return;
    }

    sf::Sprite background(backgroundTex);
    background.setScale(
        static_cast<float>(window.getSize().x) / backgroundTex.getSize().x,
        static_cast<float>(window.getSize().y) / backgroundTex.getSize().y
        );

    // Top bar, to show the player name, score and time
    sf::RectangleShape topBar(sf::Vector2f(window.getSize().x, 30));
    topBar.setPosition(0, 0);
    topBar.setFillColor(sf::Color::Black);

    // Vector of trees
    std::vector<sf::Sprite> trees;
    for (int i = 0; i < 6; ++i) {
        sf::Sprite tree(treeTex);
        tree.setScale(0.2f, 0.2f);
        tree.setPosition(50.f + i * 150.f, 400.f);
        trees.push_back(tree);
    }

    sf::Sprite basket(basketTex);
    basket.setTextureRect(sf::IntRect(4, 10, 42, basketTex.getSize().y - 20));
    basket.setScale(2.5f, 2.5f);
    basket.setPosition(512, 650);

    // Vetor of apples
    std::vector<sf::Sprite> apples;
    sf::Sprite firstApple(appleTex);
    firstApple.setTextureRect(sf::IntRect(10, 9, 30, appleTex.getSize().y - 11));
    firstApple.setPosition(randomX(window.getSize().x - 50), 50);
    apples.push_back(firstApple);

    // Texts for the top bar and if you pause the game
    sf::Text playerText("Player: " + playerName, font, 18);
    playerText.setFillColor(sf::Color::White);
    playerText.setPosition(10, 5);

    sf::Text timeText("Time: 00:00", font, 18);
    timeText.setFillColor(sf::Color::White);

    sf::Text scoreText("Score: 0", font, 18);
    scoreText.setFillColor(sf::Color::White);

    sf::Text pauseText("PAUSED\nPress SPACE to resume", font, 48);
    pauseText.setFillColor(sf::Color::Yellow);
    pauseText.setOutlineColor(sf::Color::Black);
    pauseText.setOutlineThickness(2);
    pauseText.setPosition(250, 300);

    //Clock, speed of the basket and apples
    sf::Clock gameClock, appleClock;
    int score = 0;
    int applesToSpawn = 1;
    float basketSpeed = 400.f;
    float baseAppleSpeed = 200.f;

    while (window.isOpen()) {
        sf::Time delta = clock.restart();

        // Run the window unlees SPACE is pressed
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
                paused = !paused;
        }

        if (!paused) {
            // Basket movement
            sf::Vector2f pos = basket.getPosition();
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && pos.x > 0)
                basket.move(-basketSpeed * delta.asSeconds(), 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && pos.x + basket.getGlobalBounds().width < window.getSize().x)
                basket.move(basketSpeed * delta.asSeconds(), 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && pos.y > 30)
                basket.move(0, -basketSpeed * delta.asSeconds());
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && pos.y + basket.getGlobalBounds().height < window.getSize().y)
                basket.move(0, basketSpeed * delta.asSeconds());

            // Every 10 sec, adds one more apple to the game
            if (appleClock.getElapsedTime().asSeconds() >= 10.f && applesToSpawn < 6) {
                sf::Sprite newApple(appleTex);
                newApple.setTextureRect(sf::IntRect(10, 9, 30, appleTex.getSize().y - 11));
                newApple.setPosition(randomX(window.getSize().x - 50), 50);
                apples.push_back(newApple);
                applesToSpawn++;
                appleClock.restart();
            }

            // Icrease falling velocity over time
            float elapsedSeconds = gameClock.getElapsedTime().asSeconds();
            float currentAppleSpeed = baseAppleSpeed + elapsedSeconds * 4.0f;

            // A new apple spawns if it leaves the screen or if it touches the basket
            for (auto& apple : apples) {
                apple.move(0, currentAppleSpeed * delta.asSeconds());

                if (apple.getPosition().y > window.getSize().y)
                    apple.setPosition(randomX(window.getSize().x - 50), 50);

                if (basket.getGlobalBounds().intersects(apple.getGlobalBounds())) {
                    score++;
                    apple.setPosition(randomX(window.getSize().x - 50), 50);
                }
            }

            // Update the time and score texts
            int seconds = static_cast<int>(elapsedSeconds);
            timeText.setString("Time: " + std::to_string(seconds));
            scoreText.setString("Score: " + std::to_string(score));

            timeText.setPosition((window.getSize().x - timeText.getLocalBounds().width) / 2, 5);
            scoreText.setPosition(window.getSize().x - scoreText.getLocalBounds().width - 30, 5);

            // After 60 sec the game closes and opens the game over window
            if (seconds >= 60) {
                window.close();
                showGameOverScreen(font, playerName, score);
                return;
            }
        }

        // Draw all textures and texts
        window.clear();
        window.draw(background);
        for (auto& tree : trees) window.draw(tree);
        window.draw(topBar);
        window.draw(basket);
        for (auto& apple : apples) window.draw(apple);
        window.draw(playerText);
        window.draw(timeText);
        window.draw(scoreText);
        if (paused) window.draw(pauseText);
        window.display();
    }
}

void showGameOverScreen(sf::Font& font, const std::string& playerName, int score) {
    // Determine the biggest score
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
                continue;
            }
        }
        inFile.close();
    }

    enum class RecordStatus { None, Equal, New };
    RecordStatus recordStatus = RecordStatus::None;

    if (score > maxScore) {
        recordStatus = RecordStatus::New;
    } else if (score == maxScore && maxScore > 0) {
        recordStatus = RecordStatus::Equal;
    }

    sf::RenderWindow overWindow(sf::VideoMode(1024, 768), "Catcher - Game Over");

    // Texture and texts loading
    sf::Texture backgroundTex;
    if (!backgroundTex.loadFromFile("background.png")) {
        std::cerr << "Erro ao carregar background.png\n";
    }
    sf::Sprite background(backgroundTex);
    background.setColor(sf::Color(255, 255, 255, 128)); // transparência
    background.setScale(
        static_cast<float>(overWindow.getSize().x) / backgroundTex.getSize().x,
        static_cast<float>(overWindow.getSize().y) / backgroundTex.getSize().y
        );

    sf::Text gameOverText("Game Over", font, 60);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setPosition(360, 150);
    gameOverText.setOutlineColor(sf::Color::Black);
    gameOverText.setOutlineThickness(2);

    sf::Text playerText("Player: " + playerName, font, 50);
    playerText.setFillColor(sf::Color::White);
    playerText.setPosition(360, 250);

    sf::Text scoreText("Score: " + std::to_string(score), font, 40);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(440, 350);

    sf::Text recordText;
    recordText.setFont(font);
    recordText.setCharacterSize(32);
    recordText.setOutlineColor(sf::Color::Black);
    recordText.setOutlineThickness(2);
    recordText.setPosition(440, 420);

    // Conditions that determine if you did a highscore
    if (recordStatus == RecordStatus::New) {
        recordText.setString("NEW RECORD!");
        recordText.setFillColor(sf::Color::Green);
    } else if (recordStatus == RecordStatus::Equal) {
        recordText.setString("You matched the high score!");
        recordText.setFillColor(sf::Color::Cyan);
    } else {
        recordText.setString("High score: " + std::to_string(maxScore));
        recordText.setFillColor(sf::Color(200, 200, 200));
    }

    sf::Text continueText("Press Enter to return to menu", font, 30);
    continueText.setFillColor(sf::Color::Yellow);
    continueText.setPosition(320, 500);

    // Only leaves the window when enter is pressed
    while (overWindow.isOpen()) {
        sf::Event event;
        while (overWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                overWindow.close();

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                overWindow.close();
        }

        overWindow.clear();
        overWindow.draw(background);
        overWindow.draw(gameOverText);
        overWindow.draw(playerText);
        overWindow.draw(scoreText);
        overWindow.draw(recordText);
        overWindow.draw(continueText);
        overWindow.display();
    }

    // Save score and close (weather the result is)
    std::ofstream outFile("bestScores.txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << playerName << " " << score << "\n";
        outFile.close();
    } else {
        std::cerr << "Failed to save score to bestScores.txt\n";
    }
}


std::string askPlayerName(sf::Font& font) {
    sf::RenderWindow inputWindow(sf::VideoMode(1024, 768), "Catcher - Name");

    // Texture and texts loading
    sf::Texture backgroundTex;
    if (!backgroundTex.loadFromFile("background.png")) {
        std::cerr << "Erro ao carregar background.png\n";
        return "";
    }

    sf::Sprite background(backgroundTex);
    background.setColor(sf::Color(255, 255, 255, 128));
    background.setScale(
        static_cast<float>(inputWindow.getSize().x) / backgroundTex.getSize().x,
        static_cast<float>(inputWindow.getSize().y) / backgroundTex.getSize().y
        );

    sf::Text prompt("Write your name (max 20 characters) and press ENTER:", font, 24);
    prompt.setPosition(20, 40);
    prompt.setFillColor(sf::Color::White);

    // Input text for the player name
    sf::Text inputText("", font, 30);
    inputText.setPosition(20, 100);
    inputText.setFillColor(sf::Color::Yellow);

    std::string playerName;

    //The game doesn't allow you to play without a name or with a name bigger than 20 caracters
    while (inputWindow.isOpen()) {
        sf::Event event;
        while (inputWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                inputWindow.close();
                return "";
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') {
                    if (!playerName.empty()) {
                        playerName.pop_back();
                    }
                } else if ((event.text.unicode == '\r' || event.text.unicode == '\n') && !playerName.empty()) {
                    inputWindow.close();
                } else if (event.text.unicode < 128 && isprint(event.text.unicode)) {
                    if (playerName.size() < 20) {
                        playerName += static_cast<char>(event.text.unicode);
                    }
                }
                inputText.setString(playerName);
            }
        }

        inputWindow.clear();
        inputWindow.draw(background);
        inputWindow.draw(prompt);
        inputWindow.draw(inputText);
        inputWindow.display();
    }

    return playerName;
}



void showCredits(sf::Font& font, const sf::Texture& backgroundTexture) {
    //Loading textures and texts
    sf::Sprite background(backgroundTexture);
    background.setColor(sf::Color(255, 255, 255, 128));
    sf::Vector2u textureSize = backgroundTexture.getSize();
    background.setScale(1024.f / textureSize.x, 768.f / textureSize.y);

    sf::RenderWindow credits(sf::VideoMode(1024, 768), "Catcher - Credits");

    sf::Text title("Credits", font, 48);
    title.setFillColor(sf::Color::White);
    title.setPosition(50, 20);

    sf::Text text("Game developed by Rafael Louro with the help of AI tools\n"
                  "Basket and apple designed by Rafael Louro at https://www.pixilart.com\n"
                  "Other textures from https://www.freepik.com", font, 24);
    text.setFillColor(sf::Color::White);
    text.setPosition(50, 120);

    sf::Text backText("Press ENTER to return to the menu", font, 30);
    backText.setFillColor(sf::Color::Yellow);
    backText.setPosition(50, 650);

    // Print everything and only leave the window when enter is pressed
    while (credits.isOpen()) {
        sf::Event e;
        while (credits.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                credits.close();
            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter)
                credits.close();
        }

        credits.clear();
        credits.draw(background);
        credits.draw(title);
        credits.draw(text);
        credits.draw(backText);
        credits.display();
    }
}

void showRules(sf::Font& font, const sf::Texture& backgroundTexture) {
    sf::Sprite background(backgroundTexture);
    background.setColor(sf::Color(255, 255, 255, 128));
    sf::Vector2u textureSize = backgroundTexture.getSize();
    background.setScale(1024.f / textureSize.x, 768.f / textureSize.y);

    sf::RenderWindow rules(sf::VideoMode(1024, 768), "Catcher - Rules");

    sf::Text title("Game Rules", font, 48);
    title.setFillColor(sf::Color::White);
    title.setPosition(50, 20);

    sf::Text text("Rules:\n"
                  " - Catch the fruits with your basket!\n"
                  " - Move the basket using the arrow keys, you can move in all directions!\n"
                  " - You have 60 seconds to get as many points as possible\n"
                  " - You can pause the game using SPACE\n"
                  " - Good luck! :)", font, 24);
    text.setFillColor(sf::Color::White);
    text.setPosition(50, 120);

    sf::Text backText("Press ENTER to return to the menu", font, 30);
    backText.setFillColor(sf::Color::Yellow);
    backText.setPosition(50, 650);

    while (rules.isOpen()) {
        sf::Event e;
        while (rules.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                rules.close();
            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter)
                rules.close();
        }

        rules.clear();
        rules.draw(background);
        rules.draw(title);
        rules.draw(text);
        rules.draw(backText);
        rules.display();
    }
}


int main() {
    //Load the font of the texts
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Failed to load arial.ttf\n";
        return 1;
    }

    // Background loading - done here bc it is used in all windows
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("background.png")) {
        std::cerr << "Failed to load background.png\n";
        return 1;
    }
    sf::Sprite background(backgroundTexture);
    background.setColor(sf::Color(255, 255, 255, 128)); //128 - opacity 50%

    sf::Vector2u textureSize = backgroundTexture.getSize();
    float scaleX = 1024.f / textureSize.x;
    float scaleY = 768.f / textureSize.y;
    background.setScale(scaleX, scaleY);


    while (true) {
        sf::RenderWindow menu(sf::VideoMode(1024, 768), "Catcher - Main Menu");

        sf::Text title("Catch Game", font, 48);
        title.setFillColor(sf::Color::Yellow);
        title.setOutlineColor(sf::Color::Black);
        title.setOutlineThickness(2);
        title.setPosition(512 - title.getLocalBounds().width / 2, 100);    //Make the text centered

        const float windowWidth = 1024;
        const float buttonWidth = 250;
        const float buttonHeight = 60;
        const float buttonMiddleX = (windowWidth - buttonWidth) / 2; // 768/2 - to make buttons centered

        //Load the buttons
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

        // Load the texts
        sf::Text playText("Play", font, 30);
        playText.setFillColor(sf::Color::Black);
        playText.setPosition(
            buttonMiddleX + (buttonWidth - playText.getLocalBounds().width) / 2, 235); // To make the text centered

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

        // Infinite loop that checks if the mouse was pressed in any of the buttons and calls the functions
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
                        std::string playerName = askPlayerName(font);
                        if (!playerName.empty()) {
                            runGame(font, playerName);
                        }
                        menu.create(sf::VideoMode(1024, 768), "Main Menu");
                    }
                    else if (topScoresButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        showTopScores(font, backgroundTexture);
                        menu.create(sf::VideoMode(1024, 768), "Main Menu"); // Recria o menu após fechar a janela Top Scores
                    }
                    else if (quitButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        return 0; // Termina o programa ao clicar "Sair do Jogo"
                    }
                    else if (creditsButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        showCredits(font, backgroundTexture);
                    }
                    else if (rulesButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        menu.close();
                        showRules(font, backgroundTexture);
                    }
                    else{
                    menu.create(sf::VideoMode(1024, 768), "Main Menu");
                    }
                }
            }
            // Drawing everything
            menu.clear();
            menu.draw(background);
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

