#include "game_server.hpp"
#include "network_protocol.hpp"
#include "utility.hpp"
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Sleep.hpp>
#include <iostream>
#include "pointbox.hpp"

// ---------------------------------------------------------------------------
// Construction / destruction
// Function made with the use of AI to work for our game
// ---------------------------------------------------------------------------

GameServer::GameServer(sf::Vector2f battlefield_size)
    : m_thread(&GameServer::ExecutionThread, this)
    , m_listening_state(false)
    , m_client_timeout(sf::seconds(5.f))
    , m_max_connected_players(16)
    , m_connected_players(0)
    , m_battlefield_rect(sf::Vector2f(0.f, 0.f), battlefield_size)
    , m_aircraft_count(0)
    , m_peers(1)
    , m_aircraft_identifier_counter(1)
    , m_waiting_thread_end(false)
    , m_winner_announced(false)
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

    while (!m_waiting_thread_end)
    {
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
    static int tick_count = 0;
    static sf::Clock tick_report_clock;
    tick_count++;
    if (tick_report_clock.getElapsedTime() >= sf::seconds(1.f)) {
        std::cout << "[SERVER] [TICK] Tick rate observed: " << tick_count << " ticks/sec\n\n";
        tick_count = 0;
        tick_report_clock.restart();
    }

    // Broadcast a fresh state snapshot to all clients
    UpdateClientState();

	const sf::Time tick_rate = sf::seconds(1.f / 20.f);
	m_crate_spawn_timer += tick_rate;

    if (m_crate_spawn_timer >= m_crate_spawn_interval)
    {
        m_crate_spawn_timer = sf::Time::Zero;
		m_crate_spawn_interval = sf::seconds(1.f + static_cast<float>(std::rand() % 5)); // Random interval between 1 and 5 seconds

        uint8_t type_idx = static_cast<uint8_t>(Utility::RandomInt(static_cast<int>(PointBoxType::kPointBoxCount)));
        float spawn_x = 50.f + static_cast<float>(std::rand() % static_cast<int>(m_battlefield_rect.size.x) - 100.f);

        sf::Packet packet;
        packet << static_cast<uint8_t>(Server::PacketType::kSpawnPickup)
            << type_idx
            << spawn_x;
		SendToAll(packet);
    }

    // ---- Crate Scott win condition: first aircraft to reach 30 points ----
    if (!m_winner_announced)
    {
        for (const auto& kv : m_aircraft_info)
        {
            if (kv.second.m_score >= 30)
            {
                sf::Packet packet;
                packet << static_cast<uint8_t>(Server::PacketType::kMissionSuccess);
                SendToAll(packet);
                m_winner_announced = true;
                std::cout << "Server: Aircraft " << +kv.first << " reached 30 - sending MissionSuccess\n";
                break;
            }
        }
    }
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

    // Assign spawn position: P1 centre-left, P2 slightly right (mirrors local game)
    float spawn_x = m_battlefield_rect.size.x / 2.f
        + static_cast<float>(m_connected_players) * 100.f;
    float spawn_y = m_battlefield_rect.size.y - 200.f;

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
    //AI
    std::size_t pkt_size = packet.getDataSize();
    std::cout << "[SERVER] [PACKET] SendToAll packet size=" << pkt_size << " bytes\n\n";


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
