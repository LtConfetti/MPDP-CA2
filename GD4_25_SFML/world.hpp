#pragma once
#include <SFML/Graphics.hpp>
#include "resource_identifiers.hpp"
#include "scene_node.hpp"
#include "scene_layers.hpp"
#include "aircraft.hpp"
#include "sound_player.hpp" //Ben Arrowsmith
#include "command_queue.hpp"
#include "pointbox.hpp"
#include "pointbox_type.hpp"
#include "bloom_effect.hpp"

//Ben Arrowsmith D00257746
//John Nally D00258753
class World
{
public:
	explicit World(sf::RenderWindow& window, SoundPlayer& sound, FontHolder& font); //Ben Arrowsmith
	void Update(sf::Time dt);
	void Draw();

	CommandQueue& GetCommandQueue();

	bool HasPlayerReachedPoints() const; //John Nally: New Win Condition for Points

	int GetPlayer1Score() const; //John Nally: Player1 Get Points
	int GetPlayer2Score() const; //John Nally: Player 2 Get Points
	int GetWinningPlayer() const; //John Nally: Winner for GameState

private:
	void LoadTextures();
	void BuildScene();
	void AdaptPlayerVelocity();
	void AdaptPlayerPosition();

	sf::FloatRect GetViewBounds() const;
	sf::FloatRect GetBattleFieldBounds() const;

	void HandleCollisions();

	void DestroyEntitiesOutsideView();

	void SpawnPointBoxes();
	void UpdatePointBoxSpawning(sf::Time dt);

	void UpdateSounds(); //Ben Arrowsmith

private:
	sf::RenderWindow& m_window;
	sf::View m_camera;
	TextureHolder m_textures;
	FontHolder& m_fonts;
	SoundPlayer& m_sounds; //Ben Arrowsmith
	SceneNode m_scene_graph;
	std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
	sf::FloatRect m_world_bounds;
	sf::Vector2f m_spawn_position;
	sf::Vector2f m_spawn_position2;
	Aircraft* m_player_aircraft;
	Aircraft* m_player_aircraft2; //Ben Arrowsmith

	sf::Time m_pointbox_spawn_timer; //Timer to track when to spawn the boxes
	int m_player1_score; //Score for player 1
	int m_player2_score; //Score for player 2

	CommandQueue m_command_queue;

	sf::RenderTexture m_scene_texture; //For Bloom Effect
	BloomEffect m_bloom_effect;
};
