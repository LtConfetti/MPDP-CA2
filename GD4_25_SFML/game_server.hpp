#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <thread>
#include <cstdint>
#include <map>

//This class was made with Claude AI
// John Nally D00258753
//functions that were written or heavily edited by users will be marked with a comment in the function headers, but all functions were lightly fixed to fix bugs in the AI's code. 
class GameServer
{
public:
	explicit GameServer(sf::Vector2f battlefield_size);
	~GameServer();
	void NotifyPlayerSpawn(uint8_t aircraft_identifier);
	void NotifyPlayerRealtimeChange(uint8_t aircraft_identifier, uint8_t action, bool action_enabled);
	void NotifyPlayerEvent(uint8_t aircraft_identifier, uint8_t action);

private:
	struct RemotePeer
	{
		RemotePeer();
		sf::TcpSocket m_socket;
		sf::Time m_last_packet_time;
		std::vector<uint8_t> m_aircraft_identifiers;
		bool m_ready;
		bool m_timed_out;
	};

	struct AircraftInfo
	{
		sf::Vector2f m_position;
		uint8_t m_hitpoints;
		int m_score; //John Nally D00258753: Added score tracking to server for win condition and client UI
	};

	typedef std::unique_ptr<RemotePeer> PeerPtr;

private:
	void SetListening(bool enable);
	void ExecutionThread();
	void Tick();
	sf::Time Now() const;

	void HandleIncomingPackets();
	void HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout);

	void HandleIncomingConnections();
	void HandleDisconnections();

	void InformWorldState(sf::TcpSocket& socket);
	void BroadcastMessage(const std::string& message);
	void SendToAll(sf::Packet& packet);
	void UpdateClientState();

private:
	std::thread m_thread;
	sf::Clock m_clock;
	sf::TcpListener m_listener_socket;
	bool m_listening_state;
	sf::Time m_client_timeout;

	std::size_t m_max_connected_players;
	std::size_t m_connected_players;

	float m_world_height;
	sf::FloatRect m_battlefield_rect;
	float m_battlefield_scrollspeed;

	std::size_t m_aircraft_count;
	std::map<uint8_t, AircraftInfo> m_aircraft_info;

	std::vector<PeerPtr> m_peers;
	uint8_t m_aircraft_identifier_counter;
	bool m_waiting_thread_end;

	sf::Time m_crate_spawn_timer;
	sf::Time m_crate_spawn_interval;

	bool  m_winner_announced;

	// 60-second game timer John Nally D00258753 
	float m_game_seconds_left;      // counts down from 60
	int   m_last_timer_broadcast;   // last whole second we broadcast
	bool  m_game_over_sent;         // prevent double kMissionSuccess

	// Lobby countdown
	bool     m_lobby_active;      // true while countdown is running
	float    m_lobby_seconds_left; // counts down from 15 to 0
	int      m_last_broadcast_second; // tracks which whole second we last broadcast
};

