#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <cmath>
#include <set>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdint>

class Player : public sf::CircleShape {
  public:
    Player(float width, float height, sf::Color color) :
      sf::CircleShape(width / 2.f), //(sf::Vector2f(width, height)),
      m_orientation(sf::Vector2f(1, 0)),
      m_velocity(sf::Vector2f(0, 0)),
      m_position(sf::Vector2f(0, 0)),
      m_originalColor(color)
    {
        setFillColor(color);
        setOrigin(sf::Vector2f(width / 2.f, width / 2.f));
        m_speed = 100.;
        m_isHit = false;
        m_hitTime = 0;
    }

    void hit() {
        m_isHit = true;
    }

    void update(float delta) {
        if (m_isHit) {
            setFillColor(sf::Color::Red);
            m_hitTime = 0;
        }
        if (m_hitTime > 1.) {
            setFillColor(m_originalColor);
        }
        m_hitTime += delta;
        m_isHit = false;
    }

    void move(sf::Vector2f vector) {
        sf::CircleShape::move(vector);
        m_position += vector;
    }

  public:
    sf::Color m_originalColor;
    float m_hitTime;
    bool m_isHit;
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
        setOrigin(sf::Vector2f(radius, radius));
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


sf::Packet& operator<<(sf::Packet& packet, const Player& player)
{
    packet << player.m_position.x << player.m_position.y;
    packet << player.m_orientation.x << player.m_orientation.y;
    return packet;
}

sf::Packet& operator>>(sf::Packet& packet, Player& player)
{
    packet >> player.m_position.x >> player.m_position.y;
    packet >> player.m_orientation.x >> player.m_orientation.y;
    return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const Bullet& bullet)
{
    packet << bullet.m_position.x << bullet.m_position.y;
    packet << bullet.m_velocity.x << bullet.m_velocity.y;
    return packet;
}

sf::Packet& operator>>(sf::Packet& packet, Bullet& bullet)
{
    packet >> bullet.m_position.x >> bullet.m_position.y;
    packet >> bullet.m_velocity.x >> bullet.m_velocity.y;
    return packet;
}

float norm(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

bool collision(sf::CircleShape circle1, sf::CircleShape circle2)
{
    // returns true if they collide
    return (circle1.getRadius() + circle2.getRadius()) > norm(circle2.getPosition() - circle1.getPosition());
}

int server(int port)
{
    sf::UdpSocket socket;
    if (socket.bind(port) != sf::Socket::Done) {
        std::cout << "Server: UdpSocket::bind failed." << std::endl;
        return 1;
    }

    std::cout << "Server: Server started." << std::endl;
    
    std::set<std::pair<sf::IpAddress, unsigned short>> senderPairList;
    sf::Packet packet;
    sf::IpAddress senderAddress;
    unsigned short senderPort;

    while (true)
    {
        if (socket.receive(packet, senderAddress, senderPort) != sf::Socket::Done) {
            std::cout << "Server: UdpSocket::receive failed." << std::endl;
            continue;
        }

        // Packet received
        auto senderPair = std::make_pair(senderAddress, senderPort);
        auto isInSet = senderPairList.insert(senderPair);

        if (isInSet.second) {
            std::cout << "Server: New client:" << senderAddress << ":" << senderPort << std::endl;
        } else {
            std::cout << "Server: Packet from known client:" << senderAddress << ":" << senderPort << std::endl;
        }

        // Send to all clients except sender
        for (auto receiver : senderPairList) {
            if (receiver != senderPair) {
                if (socket.send(packet, receiver.first, receiver.second) != sf::Socket::Done) {
                    std::cout << "Server: UdpSocket::send failed." << std::endl;
                }
            }
        }
    }
    std::cout << "Server: I ripped." << std::endl;
    return 1;
}

int main()
{
    sf::IpAddress ip("127.0.0.1");
    unsigned short port = 6768;

    // Start local server
    std::thread serverThread(server, port);


    // Connect to server
    sf::UdpSocket socket;
    socket.setBlocking(true);
    sf::Packet packet;

    packet << "hello";
    socket.send(packet, ip, port);

    socket.setBlocking(false);

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
    float packetCooldown = 1.f; // seconds
    float packetTimer = packetCooldown;
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
                    bullets.push_back(new Bullet(5, sf::Color::Red, character.m_orientation, character.m_position + (character.getRadius() + 5.f) * character.m_orientation));
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
        float mouseOrientationLength = norm(mouseOrientation);
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


        
        while (true)
        {
            sf::IpAddress receivedIp;
            unsigned short receivedPort;

            // receive packet
            packet.clear();

            sf::Socket::Status status = socket.receive(packet, receivedIp, receivedPort);

            // Done, NotReady, Error, Disconnected

            if (status == sf::Socket::NotReady) {
                break;
            } else if (status == sf::Socket::Error) {
                std::cout << "Client: sf::UdpSocket::Error while receiving packet" << std::endl;
                break;
            } else if (status == sf::Socket::Disconnected) {
                std::cout << "Client: sf::UdpSocket::Disconnected what the fuck even happend" << std::endl;
                break;
            }

            // if from our server
            if (ip == receivedIp && port == receivedPort) {
                std::cout << "Client: sf::UdpSocket::Done received valid packet" << std::endl;
                //packet << ;
            }
        }

        // draw the frame
        window.clear();
        
        bool playerCollided = false;
        
        // Bullets
        for (auto bullet : bullets) {
            bullet->update(delta);
            playerCollided = collision(character, *bullet);

            window.draw(*bullet);
        }
        if (playerCollided)
        {
            character.hit();
        }

        character.update(delta);
        // Player
        window.draw(character);

        // Overlay
        window.draw(text);

        // Send packet
        packet.clear();

        
        packet << static_cast<std::uint16_t>(bullets.size()) << character;
        for (auto bullet : bullets) {
            packet << *bullet;
        }
        packetTimer -= delta;
        if (packetTimer<= 0) {
            sf::Socket::Status status = socket.send(packet, ip, port);
            if (status == sf::Socket::Done) {
                std::cout << "Client: sf::UdpSocket::Done successfully sent package" << std::endl;
            }
            packetTimer = packetCooldown;
        }
        
        window.display();
    }

    std::cout << "Server still running." << std::endl;
    serverThread.join();

    return 0;
}
