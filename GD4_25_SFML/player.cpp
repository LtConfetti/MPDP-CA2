#include "player.hpp"
#include "aircraft.hpp"
#include "network_protocol.hpp"
#include <SFML/Network/Packet.hpp>

// Functor: moves the aircraft that matches m_identifier
//AI WAS USED IN THE RESTRUCTURING OF THIS FUNCTION
struct AircraftMover
{
    AircraftMover(float vx, float vy, int identifier)
        : velocity(vx, vy), aircraft_id(identifier) {}

    void operator()(Aircraft& aircraft, sf::Time) const
    {
        if (aircraft.GetIdentifier() == static_cast<uint8_t>(aircraft_id))
            aircraft.Accelerate(velocity * aircraft.GetMaxSpeed());
    }

    sf::Vector2f velocity;
    int          aircraft_id;
};

// Functor: fires the aircraft that matches m_identifier
struct AircraftFireTrigger
{
    AircraftFireTrigger(int identifier) : aircraft_id(identifier) {}

    void operator()(Aircraft& aircraft, sf::Time) const
    {
        if (aircraft.GetIdentifier() == static_cast<uint8_t>(aircraft_id))
            aircraft.Fire();
    }

    int aircraft_id;
};

// -------------------------------------------------------------------------

Player::Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding)
    : m_key_binding(binding)
    , m_current_mission_status(MissionStatus::kMissionRunning)
    , m_identifier(identifier)
    , m_socket(socket)
	, m_winner_id(0)
{
    InitialiseActions();

    // All commands target kPlayerAircraft; the functors filter by identifier,
    // so both players can share the same category without affecting each other.
    for (auto& pair : m_action_binding)
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft)
        | static_cast<unsigned int>(ReceiverCategories::kPlayer2Aircraft);
}

// ---- Local keyboard input -----------------------------------------------

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    // Only local players process keyboard events
    if (!m_key_binding)
        return;

    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        Action action;
        if (m_key_binding->CheckAction(key_pressed->scancode, action) && !IsRealtimeAction(action))
        {
            if (m_socket)
            {
                // Networked: send one-shot event to server
                sf::Packet packet;
                packet << static_cast<uint8_t>(Client::PacketType::kPlayerEvent);
                packet << m_identifier;
                packet << static_cast<uint8_t>(action);
                m_socket->send(packet);
            }
            else
            {
                // Local-only: push directly
                command_queue.Push(m_action_binding[action]);
            }
        }
    }

    // Track key press/release for realtime actions
    struct KeyStatus { sf::Keyboard::Scancode code; bool isPressed; };
    std::optional<KeyStatus> keyData;
    if (const auto* press = event.getIf<sf::Event::KeyPressed>())
        keyData = { press->scancode, true };
    else if (const auto* rel = event.getIf<sf::Event::KeyReleased>())
        keyData = { rel->scancode, false };

    if (keyData && m_socket)
    {
        Action action;
        if (m_key_binding->CheckAction(keyData->code, action) && IsRealtimeAction(action))
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(action);
            packet << keyData->isPressed;
            m_socket->send(packet);
        }
    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    if (!m_key_binding)
        return;

    // In a networked game only local players drive realtime commands this way.
    // Remote players are driven via HandleRealtimeNetworkInput instead.
    if ((m_socket && IsLocal()) || !m_socket)
    {
        for (Action action : m_key_binding->GetRealtimeActions())
            command_queue.Push(m_action_binding[action]);
    }
}

// ---- Network-driven input (remote players) --------------------------------

void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    // Only remote networked players use this path
    if (!m_socket || IsLocal())
        return;

    for (auto& pair : m_action_proxies)
        if (pair.second && IsRealtimeAction(pair.first))
            commands.Push(m_action_binding[pair.first]);
}

void Player::HandleNetworkEvent(Action action, CommandQueue& commands)
{
    commands.Push(m_action_binding[action]);
}

void Player::HandleNetworkRealtimeChange(Action action, bool actionEnabled)
{
    m_action_proxies[action] = actionEnabled;
}

// ---- Pause / disconnect helpers -------------------------------------------

void Player::DisableAllRealtimeActions(bool enable)
{
    if (!m_socket)
        return;
    for (auto& pair : m_action_proxies)
    {
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
        packet << m_identifier;
        packet << static_cast<uint8_t>(pair.first);
        packet << enable;
        m_socket->send(packet);
    }
}

bool Player::IsLocal() const
{
    // A player with no key binding is a remotely-controlled aircraft
    return m_key_binding != nullptr;
}

// ---- Mission status -------------------------------------------------------

void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

// ---- Action wiring --------------------------------------------------------

void Player::InitialiseActions()
{
    // Use identifier-filtered functors so both players' commands can safely
    // broadcast to kPlayerAircraft | kPlayer2Aircraft without interfering.
    m_action_binding[Action::kMoveLeft].action =
        DerivedAction<Aircraft>(AircraftMover(-1.f, 0.f, m_identifier));
    m_action_binding[Action::kMoveRight].action =
        DerivedAction<Aircraft>(AircraftMover(+1.f, 0.f, m_identifier));
    m_action_binding[Action::kMoveUp].action =
        DerivedAction<Aircraft>(AircraftMover(0.f, -1.f, m_identifier));
    m_action_binding[Action::kMoveDown].action =
        DerivedAction<Aircraft>(AircraftMover(0.f, +1.f, m_identifier));
    m_action_binding[Action::kBulletFire].action =
        DerivedAction<Aircraft>(AircraftFireTrigger(m_identifier));
}

//Mission Status and winner ID getters/setters
void Player::SetWinnerID(uint8_t id)
{
    m_winner_id = id;
}

uint8_t Player::GetWinnerID() const
{
    return m_winner_id;
}

void Player::SetIdentifier(uint8_t id)
{
    m_identifier = id;
}

uint8_t Player::GetIdentifier() const
{
    return m_identifier;
}
