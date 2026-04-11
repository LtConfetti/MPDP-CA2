#include "multiplayer_gamestate.hpp"
#include "utility.hpp"
#include "fontID.hpp"
#include "mission_status.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <fstream>
#include <iostream>

// ---------------------------------------------------------------------------
// Helper: read the server IP from ip.txt next to the executable.
// Host writes "127.0.0.1"; joiner writes the host machine's LAN IP.
// Function made with the use of AI to work for our game, with context to our features and needs.
// ---------------------------------------------------------------------------
static sf::IpAddress GetAddressFromFile()
{
    {
        std::ifstream input("ip.txt");
        std::string   ip_str;
        if (input >> ip_str)
            if (auto addr = sf::IpAddress::resolve(ip_str))
                return *addr;
    }
    // File missing / bad - create it pointing at localhost
    std::ofstream output("ip.txt");
    output << sf::IpAddress::LocalHost.toString();
    return sf::IpAddress::LocalHost;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context, bool is_host)
    : State(stack, context)
    , m_world(*context.window, *context.sounds, *context.fonts, /*networked=*/true)
    , m_window(*context.window)
    , m_connected(false)
    , m_game_server(nullptr)
    , m_active_state(true)
    , m_has_focus(true)
    , m_host(is_host)
    , m_game_started(false)
    , m_client_timeout(sf::seconds(5.f))
    , m_time_since_last_packet(sf::seconds(0.f))
    , m_broadcast_text(context.fonts->Get(FontID::kMain))
    , m_failed_connection_text(context.fonts->Get(FontID::kMain))
{
    // Broadcast message display
    m_broadcast_text.setCharacterSize(24);
    m_broadcast_text.setFillColor(sf::Color::White);
    Utility::CentreOrigin(m_broadcast_text);
    m_broadcast_text.setPosition(
        sf::Vector2f(m_window.getSize().x / 2.f, 80.f));

    // Connection status display
    m_failed_connection_text.setCharacterSize(35);
    m_failed_connection_text.setFillColor(sf::Color::White);
    m_failed_connection_text.setString("Attempting to connect...");
    Utility::CentreOrigin(m_failed_connection_text);
    m_failed_connection_text.setPosition(
        sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

    // Show "Attempting to connect..." while we block for up to 5 seconds
    m_window.clear(sf::Color::Black);
    m_window.draw(m_failed_connection_text);
    m_window.display();

    // Pre-set the failure text so we can just draw it later if needed
    m_failed_connection_text.setString("Failed to connect to server");
    Utility::CentreOrigin(m_failed_connection_text);

    // ---- Start server on host, then connect ----
    std::optional<sf::IpAddress> ip;

    if (m_host)
    {
        m_game_server.reset(new GameServer(sf::Vector2f(m_window.getSize())));
        ip = sf::IpAddress::LocalHost;
        // Give the server thread a moment to start listening
        sf::sleep(sf::milliseconds(100));
    }
    else
    {
        ip = GetAddressFromFile();
    }

    if (ip)
    {
        auto status = m_socket.connect(*ip, SERVER_PORT, sf::seconds(5.f));
        if (status == sf::Socket::Status::Done)
            m_connected = true;
        else
            m_failed_connection_clock.restart();
    }
    else
    {
        m_failed_connection_clock.restart();
    }

    m_socket.setBlocking(false);
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void MultiplayerGameState::Draw()
{
    if (m_connected)
    {
        m_world.Draw();

        // Overlay: broadcast messages in default view
        m_window.setView(m_window.getDefaultView());
        if (!m_broadcasts.empty())
            m_window.draw(m_broadcast_text);
    }
    else
    {
        m_window.setView(m_window.getDefaultView());
        m_window.draw(m_failed_connection_text);
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

bool MultiplayerGameState::Update(sf::Time dt)
{
    if (m_connected)
    {
        m_world.Update(dt);

        // ---- Local player input ----
        if (m_active_state && m_has_focus)
        {
            CommandQueue& commands = m_world.GetCommandQueue();
            for (auto& pair : m_players)
                pair.second->HandleRealTimeInput(commands);
        }

        // ---- Remote player network-driven input ----
        {
            CommandQueue& commands = m_world.GetCommandQueue();
            for (auto& pair : m_players)
                pair.second->HandleRealtimeNetworkInput(commands);
        }

        // ---- Receive packets from server ----
        sf::Packet packet;
        if (m_socket.receive(packet) == sf::Socket::Status::Done)
        {
            m_time_since_last_packet = sf::seconds(0.f);
            uint8_t packet_type;
            packet >> packet_type;
            HandlePacket(packet_type, packet);
        }
        else if (m_time_since_last_packet > m_client_timeout)
        {
            m_connected = false;
            m_failed_connection_text.setString("Lost connection to the server");
            Utility::CentreOrigin(m_failed_connection_text);
            m_failed_connection_clock.restart();
        }

        UpdateBroadcastMessage(dt);

        // ---- Send position + score update at 20 Hz ----
        if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f))
        {
            sf::Packet state_packet;
            state_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
            state_packet << static_cast<uint8_t>(m_local_player_identifiers.size());
            //AI
            static int sends_count = 0;
            static sf::Clock send_report_clock;
            sends_count++;
            if (send_report_clock.getElapsedTime() >= sf::seconds(1.f)) {
                std::cout << "[CLIENT] [SENDS] Sends observed: " << sends_count << " sends/sec\n\n";
                sends_count = 0;
                send_report_clock.restart();
            }

            for (uint8_t id : m_local_player_identifiers)
            {
                if (Aircraft* a = m_world.GetAircraft(id))
                {
                    state_packet << id
                        << a->getPosition().x
                        << a->getPosition().y
                        << static_cast<uint8_t>(a->GetHitPoints())
                        << a->GetScore();
                }
            }
            //AI
            std::size_t pkt_size = state_packet.getDataSize();
            std::cout << "[CLIENT] [PACKET] Sending state packet size=" << pkt_size << " bytes\n\n";
            m_socket.send(state_packet);
            m_socket.send(state_packet);
            m_tick_clock.restart();
        }

        m_time_since_last_packet += dt;
    }
    else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
    {
        // Connection timed out - go back to menu
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }

    return true;
}

// ---------------------------------------------------------------------------
// HandleEvent
// ---------------------------------------------------------------------------

bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
    CommandQueue& commands = m_world.GetCommandQueue();
    for (auto& pair : m_players)
        pair.second->HandleEvent(event, commands);

    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        if (key_pressed->scancode == sf::Keyboard::Scancode::Escape)
        {
            DisableAllRealtimeActions(false);
            RequestStackPush(StateID::kNetworkPause);
        }
    }
    else if (event.is<sf::Event::FocusGained>()) m_has_focus = true;
    else if (event.is<sf::Event::FocusLost>())   m_has_focus = false;

    return true;
}

// ---------------------------------------------------------------------------
// Activate / Destroy
// ---------------------------------------------------------------------------

void MultiplayerGameState::OnActivate()
{
    m_active_state = true;
}

void MultiplayerGameState::OnDestroy()
{
    if (!m_host && m_connected)
    {
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kQuit);
        m_socket.send(packet);
    }
}

void MultiplayerGameState::DisableAllRealtimeActions(bool enable)
{
    m_active_state = enable;
    for (uint8_t id : m_local_player_identifiers)
        m_players[id]->DisableAllRealtimeActions(enable);
}

// ---------------------------------------------------------------------------
// Broadcast message display
// ---------------------------------------------------------------------------

void MultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
    if (m_broadcasts.empty()) return;

    m_broadcast_elapsed_time += elapsed_time;
    if (m_broadcast_elapsed_time > sf::seconds(2.f))
    {
        m_broadcasts.erase(m_broadcasts.begin());
        if (!m_broadcasts.empty())
        {
            m_broadcast_text.setString(m_broadcasts.front());
            Utility::CentreOrigin(m_broadcast_text);
            m_broadcast_elapsed_time = sf::Time::Zero;
        }
    }
}

// ---------------------------------------------------------------------------
// Packet handling - the heart of the client-side networking
// ---------------------------------------------------------------------------

void MultiplayerGameState::HandlePacket(uint8_t packet_type, sf::Packet& packet)
{
    switch (static_cast<Server::PacketType>(packet_type))
    {

        // ---- Broadcast text message ----
    case Server::PacketType::kBroadcastMessage:
    {
        std::string message;
        packet >> message;
        m_broadcasts.push_back(message);
        if (m_broadcasts.size() == 1)
        {
            m_broadcast_text.setString(m_broadcasts.front());
            Utility::CentreOrigin(m_broadcast_text);
            m_broadcast_elapsed_time = sf::Time::Zero;
        }
    }
    break;

    // ---- Server tells this client to spawn its own aircraft ----
    case Server::PacketType::kSpawnSelf:
    {
        uint8_t      id;
        sf::Vector2f pos;
        packet >> id >> pos.x >> pos.y;

        Aircraft* aircraft = m_world.AddAircraft(id);
        aircraft->setPosition(pos);

        GetContext().player->SetIdentifier(id); // Store local identifier on player object

        // Local player: give them the appropriate key binding
        // id==1 → keys1 (WASD+Space), id==2 → keys2 (IJKL+RShift)
        const KeyBinding* binding = GetContext().keys1;

        m_players[id].reset(new Player(&m_socket, id, binding));
        m_local_player_identifiers.push_back(id);
        m_game_started = true;

        std::cout << "Client: Spawned self as aircraft " << +id
            << " at (" << pos.x << "," << pos.y << ")\n";
    }
    break;

    // ---- Another client has connected - spawn their aircraft (no key binding) ----
    case Server::PacketType::kPlayerConnect:
    {
        uint8_t      id;
        sf::Vector2f pos;
        packet >> id >> pos.x >> pos.y;

        Aircraft* aircraft = m_world.AddAircraft(id);
        aircraft->setPosition(pos);
        m_players[id].reset(new Player(&m_socket, id, nullptr));

        std::cout << "Client: Remote aircraft " << +id << " connected\n";
    }
    break;

    // ---- A client disconnected - remove their aircraft ----
    case Server::PacketType::kPlayerDisconnect:
    {
        uint8_t id;
        packet >> id;
        m_world.RemoveAircraft(id);
        m_players.erase(id);
        std::cout << "Client: Aircraft " << +id << " disconnected\n";
    }
    break;

    // ---- Initial world state (sent to joining client so it knows existing players) ----
    case Server::PacketType::kInitialState:
    {
        uint8_t aircraft_count;
        packet >> aircraft_count;

        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            uint8_t      id, hitpoints;
            int          score;
            sf::Vector2f pos;
            packet >> id >> pos.x >> pos.y >> hitpoints >> score;

            Aircraft* aircraft = m_world.AddAircraft(id);
            aircraft->setPosition(pos);
            //aircraft->SetHitpoints(hitpoints);
            if (score > 0) aircraft->AddScore(score);

            // These are all remote - no key binding
            m_players[id].reset(new Player(&m_socket, id, nullptr));
        }
    }
    break;

    // ---- Periodic position + score sync ----
    case Server::PacketType::kUpdateClientState:
    {
        uint8_t aircraft_count;
        packet >> aircraft_count;

        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            uint8_t      id, hitpoints;
            int          score;
            sf::Vector2f pos;
            packet >> id >> pos.x >> pos.y >> hitpoints >> score;

            Aircraft* aircraft = m_world.GetAircraft(id);
            if (!aircraft) continue;

            // Only update remote aircraft positionally (local aircraft are authoritative)
            bool is_local = std::find(m_local_player_identifiers.begin(),
                m_local_player_identifiers.end(), id)
                != m_local_player_identifiers.end();

            if (!is_local)
            {
                // Interpolate position for smoother movement
                sf::Vector2f interp = aircraft->getPosition()
                    + (pos - aircraft->getPosition()) * 0.1f;
                aircraft->setPosition(interp);
                //aircraft->SetHitpoints(hitpoints);

                // Sync score display: apply the delta
                int delta = score - aircraft->GetScore();
                if (delta != 0)
                    aircraft->AddScore(delta);
            }
        }
    }
    break;

    // ---- One-shot action on a remote aircraft (e.g. fire bullet) ----
    case Server::PacketType::kPlayerEvent:
    {
        uint8_t id, action;
        packet >> id >> action;

        auto itr = m_players.find(id);
        if (itr != m_players.end())
            itr->second->HandleNetworkEvent(
                static_cast<Action>(action), m_world.GetCommandQueue());
    }
    break;

    // ---- Held-key state change for a remote aircraft ----
    case Server::PacketType::kPlayerRealtimeChange:
    {
        uint8_t id, action;
        bool    enabled;
        packet >> id >> action >> enabled;

        auto itr = m_players.find(id);
        if (itr != m_players.end())
            itr->second->HandleNetworkRealtimeChange(
                static_cast<Action>(action), enabled);
    }
    break;

    // ---- Someone reached 30 points - game over ----
    // Determine winner from the aircraft scores already on each client
    case Server::PacketType::kMissionSuccess:
    {
        std::cout << "Client: MissionSuccess received\n";

        // Find which aircraft has >= 30 points to decide the winner label
        uint8_t winner_id = 0;
        int     best = -1;
        for (uint8_t id : m_local_player_identifiers)
        {
            if (Aircraft* a = m_world.GetAircraft(id))
            {
                if (a->GetScore() > best)
                {
                    best = a->GetScore();
                    winner_id = id;
                }
            }
        }
        // Also check remote aircraft
        // (we iterate all known players)
        for (auto& pair : m_players)
        {
            if (Aircraft* a = m_world.GetAircraft(pair.first))
            {
                if (a->GetScore() > best)
                {
                    best = a->GetScore();
                    winner_id = pair.first;
                }
            }
        }
      
        GetContext().player->SetWinnerID(winner_id); // storing winner ID and status on the shared player object so GameOverState can display the correct message
        GetContext().player->SetMissionStatus(MissionStatus::kPlayerXWins);

        {
            int game_number = 1; 

			std::ifstream infile("results.txt");
            std::string count_line;
            while(std::getline(infile, count_line))
                if (!count_line.empty())
                {
                    game_number++;
                }
			std::ofstream results_file("results.txt", std::ios::app);

			if (results_file.is_open())
            {
                results_file << "Game " << game_number << ": Player " << +winner_id << " wins with " << best << " points\n";
                results_file.close();
            }
        }

        {
			uint8_t local_id = GetContext().player->GetIdentifier();
            if (local_id == winner_id)
            {
                int total_wins = 0;
                {
                    std::ifstream stats("player_wins.txt");
					stats >> total_wins;
                }

				std::ofstream stats("player_wins.txt");
                if (stats.is_open())
                {
                    stats << (total_wins + 1) << "\n";
				}
            }
        }

        RequestStackPush(StateID::kGameOver);
    }
    break;

    case Server::PacketType::kSpawnPickup:
    {
        uint8_t type_idx;
        float   spawn_x;
        packet >> type_idx >> spawn_x;
        m_world.SpawnNetworkPointBox(type_idx, spawn_x);
    }
    break;

    default:
        break;
    }
}


