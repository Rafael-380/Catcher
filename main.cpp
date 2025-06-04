#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>

// Forward declaration
void showGameOverScreen(sf::Font& font, int finalScore);

void runGame(sf::Font& font) {
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Catcher Game");
    sf::Clock clock;

    sf::Texture basketTex;
    if (!basketTex.loadFromFile("basket.png")) {
        std::cerr << "Failed to load basket.png\n";
        return;
    }

    sf::Sprite basket(basketTex);
    basket.setTextureRect(sf::IntRect(0, 0, 50, basketTex.getSize().y)); // crop width
    basket.setScale(2.f, 2.f); // double size
    basket.setPosition(512, 700);

    sf::Texture appleTex;
    if (!appleTex.loadFromFile("apple.png")) {
        std::cerr << "Failed to load apple.png\n";
        return;
    }
    appleTex.setSmooth(true);
    sf::Sprite apple(appleTex);
    apple.setTextureRect(sf::IntRect(0, 0, 50, appleTex.getSize().y)); // crop width
    apple.setScale(1.f, 1.f); // normal size
    apple.setPosition(static_cast<float>(std::rand() % (1024 - 50)), 0.f);

    float speed = 300.f;
    float appleSpeed = 150.f;

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

    sf::Clock gameClock;

    int score = 0;
    int lastPrintedSecond = -1;

    while (window.isOpen()) {
        sf::Time delta = clock.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Movement
        sf::Vector2f pos = basket.getPosition();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && pos.x > 0)
            basket.move(-speed * delta.asSeconds(), 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && pos.x + basket.getGlobalBounds().width < window.getSize().x)
            basket.move(speed * delta.asSeconds(), 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && pos.y > 0)
            basket.move(0, -speed * delta.asSeconds());
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && pos.y + basket.getGlobalBounds().height < window.getSize().y)
            basket.move(0, speed * delta.asSeconds());

        // Apple falling
        apple.move(0, appleSpeed * delta.asSeconds());

        // Check collision basket & apple
        if (apple.getGlobalBounds().intersects(basket.getGlobalBounds())) {
            score++;
            apple.setPosition(static_cast<float>(std::rand() % (1024 - 50)), 0.f);
        }

        // Check if apple touches bottom of window
        if (apple.getPosition().y > window.getSize().y) {
            apple.setPosition(static_cast<float>(std::rand() % (1024 - 50)), 0.f);
        }

        // Time display & print once per second
        int seconds = static_cast<int>(gameClock.getElapsedTime().asSeconds());
        if (seconds != lastPrintedSecond) {
            lastPrintedSecond = seconds;
            std::cout << "Time: " << seconds << "s\n";
        }

        std::stringstream ssTime;
        ssTime << "Time: " << seconds;
        timeText.setString(ssTime.str());

        std::stringstream ssScore;
        ssScore << "Score: " << score;
        scoreText.setString(ssScore.str());

        // After 20 seconds, show game over screen
        if (seconds >= 20) {
            window.close();
            std::cout << "Window changed: Game -> Game Over\n";
            showGameOverScreen(font, score);
            return;
        }

        // Draw
        window.clear(sf::Color::Black);
        window.draw(basket);
        window.draw(apple);
        window.draw(timeText);
        window.draw(scoreText);
        window.display();
    }
}

void showGameOverScreen(sf::Font& font, int finalScore) {
    sf::RenderWindow overWindow(sf::VideoMode(1024, 768), "Game Over");

    sf::Text gameOver("Game Over", font, 60);
    gameOver.setFillColor(sf::Color::Red);
    gameOver.setPosition(340, 200);

    std::string scoreStr = "Score: " + std::to_string(finalScore);
    sf::Text scoreText(scoreStr, font, 40);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(440, 320);

    sf::Clock waitClock;
    while (overWindow.isOpen()) {
        sf::Event event;
        while (overWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                overWindow.close();
        }

        if (waitClock.getElapsedTime().asSeconds() >= 5) {
            overWindow.close();
            std::cout << "Window changed: Game Over -> Menu\n";
        }

        overWindow.clear(sf::Color::Black);
        overWindow.draw(gameOver);
        overWindow.draw(scoreText);
        overWindow.display();
    }
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Failed to load arial.ttf\n";
        return 1;
    }

    while (true) {
        // --- MENU WINDOW ---
        sf::RenderWindow menu(sf::VideoMode(1024, 768), "Main Menu");

        sf::Text title("Catch Game", font, 48);
        title.setPosition(350, 200);
        title.setFillColor(sf::Color::White);

        sf::RectangleShape button(sf::Vector2f(300, 80));
        button.setPosition(362, 400);
        button.setFillColor(sf::Color(100, 100, 100));

        sf::Text buttonText("Start Game", font, 32);
        buttonText.setPosition(400, 420);
        buttonText.setFillColor(sf::Color::White);

        bool startGame = false;

        std::cout << "Window changed: Menu opened\n";

        while (menu.isOpen() && !startGame) {
            sf::Event event;
            while (menu.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    return 0;

                if (event.type == sf::Event::MouseButtonPressed &&
                    event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(menu);
                    if (button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y)) {
                        startGame = true;
                        menu.close();
                        std::cout << "Window changed: Menu -> Game\n";
                    }
                }
            }

            menu.clear(sf::Color::Black);
            menu.draw(title);
            menu.draw(button);
            menu.draw(buttonText);
            menu.display();
        }

        if (startGame) {
            runGame(font);  // Game then Game Over then back to menu
        }
    }

    return 0;
}

