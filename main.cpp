#include <SFML/Graphics.hpp>
#include <cmath>


class Player : public sf::RectangleShape {
  public:
    Player(float width, float height, sf::Color color) :
      sf::RectangleShape(sf::Vector2f(width, height)),
      m_orientation(sf::Vector2f(1, 0)),
      m_velocity(sf::Vector2f(0, 0)),
      m_position(sf::Vector2f(0, 0))
    {
        setFillColor(color);
        m_speed = 100.;
    }

    void move(sf::Vector2f vector) {
        sf::RectangleShape::move(vector);
        m_position += vector;
    }

  public:
    sf::Vector2f m_orientation;
    sf::Vector2f m_velocity;
    sf::Vector2f m_position;
    float m_speed;
};


class Bullet : public sf::CircleShape {
  public:
    Bullet(float radius, sf::Color color, sf::Vector2f velocity, sf::Vector2f position):
      sf::CircleShape(radius),
      m_velocity(velocity),
      m_position(position)
    {
        setFillColor(color);
        setPosition(position);
        m_speed = 1000;
    }

    void update(float delta) {
        m_position += delta * m_speed * m_velocity;
        setPosition(m_position);
    }
  public:
    sf::Vector2f m_velocity;
    sf::Vector2f m_position;
    float m_speed;
};

int main()
{

    const auto MOVE_LEFT = sf::Keyboard::A;
    const auto MOVE_RIGHT = sf::Keyboard::D;
    const auto MOVE_UP = sf::Keyboard::W;
    const auto MOVE_DOWN = sf::Keyboard::S;
    const auto FIRE = sf::Keyboard::Space;
    const auto CLOSE = sf::Keyboard::Escape;

    sf::RenderWindow window(sf::VideoMode(800, 600), "2D Arena MMO");

    // FPS counter
    sf::Clock fpsClock;
    sf::Clock frameTime;
    std::size_t frameCounter = 0;
    sf::Font font;
    font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
    sf::Text text;
    text.setFont(font); // font is a sf::Font
    text.setCharacterSize(24); // in pixels, not points!
    text.setFillColor(sf::Color::Red);
    text.setString("0");

    // Player
    Player character(50.0f, 50.0f, sf::Color::Blue);


    // Bullet containter
    std::vector<Bullet*> bullets;

    while (window.isOpen())
    {
        float delta = frameTime.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type) {

            case sf::Event::Closed:
                window.close();
                break;
            case (sf::Event::KeyPressed):
                switch (event.key.code) {
                case CLOSE:
                    window.close();
                    break;
                case FIRE:
                    bullets.push_back(new Bullet(5, sf::Color::Red, character.m_orientation, character.m_position));
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }

        sf::Vector2f mouseOrientation(window.mapPixelToCoords(sf::Mouse::getPosition(window)) - character.m_position);
        float mouseOrientationLength = std::sqrt(mouseOrientation.x * mouseOrientation.x + mouseOrientation.y * mouseOrientation.y);
        character.m_orientation = sf::Vector2f(mouseOrientation.x / mouseOrientationLength, mouseOrientation.y / mouseOrientationLength);


        if (sf::Keyboard::isKeyPressed(MOVE_LEFT)) {
            character.move(character.m_speed * delta * sf::Vector2f(-1, 0));
        }
        if (sf::Keyboard::isKeyPressed(MOVE_RIGHT)) {
            character.move(character.m_speed * delta * sf::Vector2f(1, 0));
        }
        if (sf::Keyboard::isKeyPressed(MOVE_UP)) {
            character.move(character.m_speed * delta * sf::Vector2f(0, -1));
        }
        if (sf::Keyboard::isKeyPressed(MOVE_DOWN)) {
            character.move(character.m_speed * delta * sf::Vector2f(0, 1));
        }
        //if (sf::Keyboard::isKeyPressed(FIRE)) {
        //    bullets.push_back(new Bullet(5, sf::Color::Red, character.m_orientation, character.m_position));
        //}

        // FPS counter
        if (float elapsed = fpsClock.getElapsedTime().asSeconds() > 1) {
            char fpsString[10];
            snprintf(fpsString, 10, "%f", static_cast<float>(frameCounter) / elapsed);
            text.setString(sf::String(fpsString));
            frameCounter = 0;
            fpsClock.restart();
        }
        ++frameCounter;


        // draw the frame
        window.clear();
        window.draw(character);
        window.draw(text);

        for (auto bullet : bullets) {
            bullet->update(delta);
            window.draw(*bullet);
        }

        window.display();
    }

    return 0;
}
