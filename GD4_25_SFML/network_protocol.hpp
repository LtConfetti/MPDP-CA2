#pragma once
#include <SFML/System/Vector2.hpp>
const unsigned short SERVER_PORT = 50000; //Greater than 49151, in dynamic port range

//Added from the John Loane Branch, and then adjusted for our games gameplay loop and features, AI was used to help comment
namespace Server
{
	enum class PacketType
	{
		kBroadcastMessage,      // std::string shown on screen few seconds
		kInitialState,          // Sent to a joining client: how many aircraft + their id/pos/hp/score
		kPlayerEvent,           // One-shot action (e.g. fire bullet)
		kPlayerRealtimeChange,  // Held-key state change (moving/firing)
		kPlayerConnect,         // Another player joined - spawn their aircraft
		kPlayerDisconnect,      // A player left - remove their aircraft
		kSpawnSelf,             // Tells this client its own identifier + spawn position
		kUpdateClientState,     // Periodic position + score sync from server to all clients
		kMissionSuccess,        // Someone hit 30 points - game over
		kSpawnPickup,             // D00258753: John Nally Spawn a pickup at a position
		kLobbyCountdown,          // Claude AI: Server sends remaining lobby seconds to all clients
		kLobbyDone,               // Claude AI: Server tells all clients to start the game
		kGameTimer                // D00258753 John Nally: Server broadcasts remaining game seconds every second
	};
}

namespace Client
{
	enum class PacketType
	{
		kPlayerEvent,           // Send one-shot action to server
		kPlayerRealtimeChange,  // Send held-key state change to server
		kStateUpdate,           // Periodic position + score update sent to server
		kQuit                   // Client is cleanly disconnecting
	};
}

namespace GameActions
{
	enum Type
	{
		kEnemyExplode
	};

	struct Action
	{
		Action() = default;
		Action(Type type, sf::Vector2f position) :type(type), position(position)
		{

		}

		Type type;
		sf::Vector2f position;
	};
}