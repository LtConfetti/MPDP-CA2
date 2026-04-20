#include "game_server.hpp"
#include "network_protocol.hpp"
#include "utility.hpp"
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Sleep.hpp>
#include <iostream>
#include <cmath>
#include "pointbox.hpp"

// ---------------------------------------------------------------------------
// Construction / destruction
// Function made with the use of AI to work for our game
// ---------------------------------------------------------------------------

GameServer::GameServer(sf::Vector2f battlefield_size)
    : m_thread(&GameServer::ExecutionThread, this)
    , m_listening_state(false) //test
    , m_client_timeout(sf::seconds(5.f))
    , m_max_connected_players(16)
    , m_connected_players(0)
    , m_battlefield_rect(sf::Vector2f(0.f, 0.f), battlefield_size)
    , m_aircraft_count(0)
    , m_peers(1)
    , m_aircraft_identifier_counter(1)
    , m_waiting_thread_end(false)
    , m_winner_announced(false)
	, m_game_seconds_left(75.f) //need to add +15 seconds to account for lobby countdown, since game timer starts ticking immediately after lobby ends
    , m_last_timer_broadcast(75)
    , m_game_over_sent(false)
    , m_lobby_active(true)
    , m_lobby_seconds_left(15.f)
    , m_last_broadcast_second(15)
    , m_crate_spawn_timer(sf::Time::Zero)
    , m_crate_spawn_interval(sf::seconds(1.f))
{
    m_listener_socket.setBlocking(false);
    m_peers[0].reset(new RemotePeer());
    m_thread.detach();   // runs independently alongside the host client
}

GameServer::~GameServer()
{
    m_waiting_thread_end = true;
    // Thread is detached so we just signal it and return
}

// ---------------------------------------------------------------------------
// Outgoing notifications (called from main thread or Tick)
// ---------------------------------------------------------------------------

void GameServer::NotifyPlayerSpawn(uint8_t aircraft_identifier)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerConnect);
    packet << aircraft_identifier
        << m_aircraft_info[aircraft_identifier].m_position.x
        << m_aircraft_info[aircraft_identifier].m_position.y;
    SendToAll(packet);
}

void GameServer::NotifyPlayerRealtimeChange(uint8_t aircraft_identifier,
    uint8_t action,
    bool    action_enabled)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerRealtimeChange);
    packet << aircraft_identifier << action << action_enabled;
    SendToAll(packet);
}

void GameServer::NotifyPlayerEvent(uint8_t aircraft_identifier, uint8_t action)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerEvent);
    packet << aircraft_identifier << action;
    SendToAll(packet);
}

// ---------------------------------------------------------------------------
// Server thread
// ---------------------------------------------------------------------------

void GameServer::SetListening(bool enable)
{
    if (enable)
    {
        if (!m_listening_state)
            m_listening_state =
            (m_listener_socket.listen(SERVER_PORT) == sf::TcpListener::Status::Done);
    }
    else
    {
        m_listener_socket.close();
        m_listening_state = false;
    }
}

void GameServer::ExecutionThread()
{
    SetListening(true);

    const sf::Time tick_rate = sf::seconds(1.f / 20.f);
    sf::Time       tick_time = sf::Time::Zero;
    sf::Clock      tick_clock;

    sf::Clock lobby_clock;
    sf::Clock game_timer_clock;

    while (!m_waiting_thread_end)
    {
        // ---- Lobby countdown ----
        if (m_lobby_active)
        {
            float elapsed = lobby_clock.restart().asSeconds();
            m_lobby_seconds_left -= elapsed;

            int whole = static_cast<int>(std::ceil(m_lobby_seconds_left));
            if (whole < m_last_broadcast_second && m_connected_players > 0)
            {
                m_last_broadcast_second = whole;
                // Broadcast remaining seconds to all clients
                sf::Packet countdown_packet;
                countdown_packet << static_cast<uint8_t>(Server::PacketType::kLobbyCountdown)
                    << static_cast<uint8_t>(std::max(0, whole))
                    << static_cast<uint8_t>(m_connected_players);
                SendToAll(countdown_packet);
            }

            if (m_lobby_seconds_left <= 0.f)
            {
                m_lobby_active = false;
                SetListening(false); // stop accepting new connections
                sf::Packet done_packet;
                done_packet << static_cast<uint8_t>(Server::PacketType::kLobbyDone);
                SendToAll(done_packet);
            }
        }
        else
        {
            lobby_clock.restart(); // keep clock fresh so no huge jump if lobby re-enabled
        }

        // ---- Game timer (runs only after lobby is done) ----
        if (!m_lobby_active && !m_game_over_sent)
        {
            float elapsed = game_timer_clock.restart().asSeconds();
            m_game_seconds_left -= elapsed;

            int whole = static_cast<int>(std::ceil(m_game_seconds_left));
            if (whole < m_last_timer_broadcast && m_connected_players > 0)
            {
                m_last_timer_broadcast = whole;
                sf::Packet timer_packet;
                timer_packet << static_cast<uint8_t>(Server::PacketType::kGameTimer)
                    << static_cast<uint8_t>(std::max(0, whole));
                SendToAll(timer_packet);
            }

            if (m_game_seconds_left <= 0.f)
            {
                m_game_over_sent = true;
                // Find the player with the highest score
                uint8_t winner_id = 0;
                int     best = -1;
                for (const auto& kv : m_aircraft_info)
                {
                    if (kv.second.m_score > best)
                    {
                        best = kv.second.m_score;
                        winner_id = kv.first;
                    }
                }
                sf::Packet end_packet;
                end_packet << static_cast<uint8_t>(Server::PacketType::kMissionSuccess)
                    << winner_id;  // pack winner ID so clients don't have to guess
                SendToAll(end_packet);
                std::cout << "Server: Time up! Winner is aircraft " << +winner_id << "\n";
            }
        }
        else if (!m_lobby_active)
        {
            game_timer_clock.restart();
        }

        HandleIncomingConnections();
        HandleIncomingPackets();

        tick_time += tick_clock.getElapsedTime();
        tick_clock.restart();

        while (tick_time >= tick_rate)
        {
            Tick();
            tick_time -= tick_rate;
        }

        // Small sleep so the server thread doesn't pin a core
        sf::sleep(sf::milliseconds(50));
    }
}

void GameServer::Tick()
{
    // Broadcast a fresh state snapshot to all clients
    UpdateClientState();

    const sf::Time tick_rate = sf::seconds(1.f / 20.f);
    m_crate_spawn_timer += tick_rate;

    if (!m_lobby_active && m_crate_spawn_timer >= m_crate_spawn_interval)
    {
        m_crate_spawn_timer = sf::Time::Zero;
        m_crate_spawn_interval = sf::seconds(0.5f + static_cast<float>(std::rand() % 2)); // 0.5 to 1.5 seconds between boxes

        uint8_t type_idx = static_cast<uint8_t>(Utility::RandomInt(static_cast<int>(PointBoxType::kPointBoxCount)));
        float spawn_x = 50.f + static_cast<float>(std::rand() % static_cast<int>(m_battlefield_rect.size.x) - 100.f);

        sf::Packet packet;
        packet << static_cast<uint8_t>(Server::PacketType::kSpawnPickup)
            << type_idx
            << spawn_x;
        SendToAll(packet);
    }

    // Win condition is now time-based (60s), handled in ExecutionThread
}

sf::Time GameServer::Now() const
{
    return m_clock.getElapsedTime();
}

// ---------------------------------------------------------------------------
// Incoming packet processing
// ---------------------------------------------------------------------------

void GameServer::HandleIncomingPackets()
{
    bool detected_timeout = false;

    for (PeerPtr& peer : m_peers)
    {
        if (!peer->m_ready) continue;

        sf::Packet packet;
        while (peer->m_socket.receive(packet) == sf::Socket::Status::Done)
        {
            HandleIncomingPackets(packet, *peer, detected_timeout);
            peer->m_last_packet_time = Now();
            packet.clear();
        }

        if (Now() > peer->m_last_packet_time + m_client_timeout)
        {
            peer->m_timed_out = true;
            detected_timeout = true;
        }
    }

    if (detected_timeout)
        HandleDisconnections();
}

void GameServer::HandleIncomingPackets(sf::Packet& packet,
    RemotePeer& receiving_peer,
    bool& detected_timeout)
{
    uint8_t packet_type;
    packet >> packet_type;

    switch (static_cast<Client::PacketType>(packet_type))
    {
        // ---- Client disconnecting cleanly ----
    case Client::PacketType::kQuit:
        receiving_peer.m_timed_out = true;
        detected_timeout = true;
        break;

        // ---- One-shot action (e.g. a bullet fired) ----
    case Client::PacketType::kPlayerEvent:
    {
        uint8_t aircraft_id, action;
        packet >> aircraft_id >> action;
        NotifyPlayerEvent(aircraft_id, action);
    }
    break;

    // ---- Held-key state change ----
    case Client::PacketType::kPlayerRealtimeChange:
    {
        uint8_t aircraft_id, action;
        bool    action_enabled;
        packet >> aircraft_id >> action >> action_enabled;
        NotifyPlayerRealtimeChange(aircraft_id, action, action_enabled);
    }
    break;

    // ---- Periodic position + score update from each client ----
    case Client::PacketType::kStateUpdate:
    {
        uint8_t num_aircraft;
        packet >> num_aircraft;

        for (uint8_t i = 0; i < num_aircraft; ++i)
        {
            uint8_t      aircraft_id, hitpoints;
            int          score;
            sf::Vector2f position;
            packet >> aircraft_id >> position.x >> position.y >> hitpoints >> score;

            m_aircraft_info[aircraft_id].m_position = position;
            m_aircraft_info[aircraft_id].m_hitpoints = hitpoints;
            m_aircraft_info[aircraft_id].m_score = score;
        }
    }
    break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Connection / disconnection
// ---------------------------------------------------------------------------

void GameServer::HandleIncomingConnections()
{
    if (!m_listening_state) return;

    if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket)
        != sf::TcpListener::Status::Done)
        return;

    // Spread players across the bottom of the world
    // World is 3000 tall, camera is battlefield_rect.size.y tall
    // Spawn Y matches world.cpp: world_height - camera_height/2
    const float kWorldHeight = 3000.f;
    const float kCameraHeight = m_battlefield_rect.size.y;
    float spawn_y = kWorldHeight - kCameraHeight / 2.f;
    // Spread horizontally so players don't overlap
    float spawn_x = 80.f + static_cast<float>(m_connected_players)
        * (m_battlefield_rect.size.x - 160.f) / 15.f;

    m_aircraft_info[m_aircraft_identifier_counter].m_position = { spawn_x, spawn_y };
    m_aircraft_info[m_aircraft_identifier_counter].m_hitpoints = 100;
    m_aircraft_info[m_aircraft_identifier_counter].m_score = 0;

    // Tell this client its identifier and spawn position
    sf::Packet spawn_self_packet;
    spawn_self_packet << static_cast<uint8_t>(Server::PacketType::kSpawnSelf);
    spawn_self_packet << m_aircraft_identifier_counter;
    spawn_self_packet << m_aircraft_info[m_aircraft_identifier_counter].m_position.x;
    spawn_self_packet << m_aircraft_info[m_aircraft_identifier_counter].m_position.y;

    m_peers[m_connected_players]->m_aircraft_identifiers.push_back(
        m_aircraft_identifier_counter);

    // Inform all existing clients of the world state before we add this new one
    BroadcastMessage("New player connected!");
    InformWorldState(m_peers[m_connected_players]->m_socket);

    // Notify everyone already connected that a new aircraft has appeared
    NotifyPlayerSpawn(m_aircraft_identifier_counter++);

    m_peers[m_connected_players]->m_socket.send(spawn_self_packet);
    m_peers[m_connected_players]->m_ready = true;
    m_peers[m_connected_players]->m_last_packet_time = Now();

    m_aircraft_count++;
    m_connected_players++;

    std::cout << "Server: Player connected. Total: " << m_connected_players << "\n";

    if (m_connected_players >= m_max_connected_players)
        SetListening(false);    // room is full - stop accepting
    else
        m_peers.emplace_back(new RemotePeer());
}

void GameServer::HandleDisconnections()
{
    for (auto itr = m_peers.begin(); itr != m_peers.end();)
    {
        if ((*itr)->m_timed_out)
        {
            for (uint8_t id : (*itr)->m_aircraft_identifiers)
            {
                sf::Packet disc_packet;
                disc_packet << static_cast<uint8_t>(Server::PacketType::kPlayerDisconnect)
                    << id;
                SendToAll(disc_packet);
                m_aircraft_info.erase(id);
            }

            m_connected_players--;
            m_aircraft_count -= (*itr)->m_aircraft_identifiers.size();
            itr = m_peers.erase(itr);

            if (m_connected_players < m_max_connected_players)
            {
                m_peers.emplace_back(new RemotePeer());
                SetListening(true);
            }
            BroadcastMessage("A player has disconnected.");
        }
        else
        {
            ++itr;
        }
    }
}

// ---------------------------------------------------------------------------
// State broadcast helpers
// ---------------------------------------------------------------------------

void GameServer::InformWorldState(sf::TcpSocket& socket)
{
    // Sent to a newly connecting client so it knows about existing players
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kInitialState);
    packet << static_cast<uint8_t>(m_aircraft_count);

    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (!m_peers[i]->m_ready) continue;
        for (uint8_t id : m_peers[i]->m_aircraft_identifiers)
        {
            packet << id
                << m_aircraft_info[id].m_position.x
                << m_aircraft_info[id].m_position.y
                << m_aircraft_info[id].m_hitpoints
                << m_aircraft_info[id].m_score;
        }
    }
    socket.send(packet);
}

void GameServer::BroadcastMessage(const std::string& message)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kBroadcastMessage) << message;
    for (std::size_t i = 0; i < m_connected_players; ++i)
        if (m_peers[i]->m_ready)
            m_peers[i]->m_socket.send(packet);
}

void GameServer::SendToAll(sf::Packet& packet)
{
    for (std::size_t i = 0; i < m_connected_players; ++i)
        if (m_peers[i]->m_ready)
            m_peers[i]->m_socket.send(packet);
}

void GameServer::UpdateClientState()
{
    // 20 Hz position + score sync pushed to every connected client
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kUpdateClientState);
    packet << static_cast<uint8_t>(m_aircraft_count);

    for (const auto& kv : m_aircraft_info)
    {
        packet << kv.first
            << kv.second.m_position.x
            << kv.second.m_position.y
            << kv.second.m_hitpoints
            << kv.second.m_score;
    }
    SendToAll(packet);
}

// ---------------------------------------------------------------------------
// RemotePeer constructor
// ---------------------------------------------------------------------------

GameServer::RemotePeer::RemotePeer()
    : m_ready(false)
    , m_timed_out(false)
{
    // Non-blocking so the server thread never hangs waiting for a peer
    m_socket.setBlocking(false);
}
