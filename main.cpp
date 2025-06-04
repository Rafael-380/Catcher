#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>

// Forward declaration
void showGameOverScreen(sf::Font& font);

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

    float speed = 300.f;

    sf::Text timeText;
    timeText.setFont(font);
    timeText.setCharacterSize(24);
    timeText.setFillColor(sf::Color::White);
    timeText.setPosition(860, 10);

    sf::Clock gameClock;

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

        // Time
        int seconds = static_cast<int>(gameClock.getElapsedTime().asSeconds());
        std::stringstream ss;
        ss << "Time: " << seconds;
        timeText.setString(ss.str());

        std::cout << "Time: " << seconds << "s\n";

        // After 20 seconds, show game over screen
        if (seconds >= 20) {
            window.close();
            showGameOverScreen(font); // NEW: show game over screen
            return;
        }

        // Draw
        window.clear(sf::Color::Black);
        window.draw(basket);
        window.draw(timeText);
        window.display();
    }
}

void showGameOverScreen(sf::Font& font) {
    sf::RenderWindow overWindow(sf::VideoMode(1024, 768), "Game Over");

    sf::Text gameOver("Game Over", font, 60);
    gameOver.setFillColor(sf::Color::Red);
    gameOver.setPosition(340, 200);

    sf::Text scoreText("Score: ", font, 40);  // We'll update this later
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
            overWindow.close();  // Close after 5 seconds
        }

        overWindow.clear(sf::Color::Black);
        overWindow.draw(gameOver);
        overWindow.draw(scoreText);
        overWindow.display();
    }
}

int main() {
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
