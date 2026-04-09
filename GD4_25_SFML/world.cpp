#include "world.hpp"
#include "sprite_node.hpp"
#include <iostream>
#include "state.hpp"
#include <SFML/System/Angle.hpp>
#include "Projectile.hpp"
#include "pointbox.hpp"
#include "pointbox_type.hpp"
#include "utility.hpp"
#include "sound_node.hpp"
#include "posteffect.hpp"

//Ben Arrowsmith D00257746
//John Nally D00258753

World::World(sf::RenderWindow& window, SoundPlayer& sounds, FontHolder& font) //Ben Arrowsmith sound
	: m_window(window)
	, m_camera(window.getDefaultView())
	, m_textures()
	, m_sounds(sounds)
	, m_fonts(font)
	, m_scene_graph(ReceiverCategories::kNone)
	, m_scene_layers()
	, m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(m_camera.getSize().x, 3000.f))
	, m_spawn_position(m_camera.getSize().x / 2.f, m_world_bounds.size.y - m_camera.getSize().y/2.f)
	, m_spawn_position2(m_camera.getSize().x / 2.f + 100.f, m_world_bounds.size.y - m_camera.getSize().y / 2.f)
	, m_player_aircraft()
	, m_player_aircraft2() //Ben Arrowsmith
	, m_pointbox_spawn_timer(sf::Time::Zero) //Timer
	, m_player1_score(0) //Player 1 Score Count
	, m_player2_score(0) //Player 2 score

{
	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_spawn_position);
	m_scene_texture.resize({ window.getSize().x, window.getSize().y });
}

void World::Update(sf::Time dt)
{
	m_player_aircraft->SetVelocity(0.f, 0.f);
	m_player_aircraft2->SetVelocity(0.f, 0.f); //Ben Arrowsmith

	DestroyEntitiesOutsideView();

	UpdateSounds(); //Ben Arrowsmith

	//Process commands from the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
	}
	AdaptPlayerVelocity();

	HandleCollisions();
	m_scene_graph.RemoveWrecks();

	m_scene_graph.Update(dt, m_command_queue);
	AdaptPlayerPosition();

	UpdatePointBoxSpawning(dt);
}



void World::Draw()
{
	if (PostEffect::IsSupported())
	{
		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scene_graph);
		m_scene_texture.display();
		m_bloom_effect.Apply(m_scene_texture, m_window);
	}
	else
	{
		m_window.setView(m_camera);
		m_window.draw(m_scene_graph);
	}

}



CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

void World::LoadTextures()
{
	m_textures.Load(TextureID::kEagle, "Media/Textures/Player1.png");
	m_textures.Load(TextureID::kEagle2, "Media/Textures/Player2.png");
	m_textures.Load(TextureID::kLandscape, "Media/Textures/background.png");
	m_textures.Load(TextureID::kBullet, "Media/Textures/Bullet.png");

	//John Nally: Box Textures for Point Boxes
	m_textures.Load(TextureID::kPointBoxPlusOne, "Media/Textures/box_plus_one.png");
	m_textures.Load(TextureID::kPointBoxPlusTwo, "Media/Textures/box_plus_two.png");
	m_textures.Load(TextureID::kPointBoxPlusThree, "Media/Textures/box_plus_three.png");
	m_textures.Load(TextureID::kPointBoxMinusFive, "Media/Textures/box_minus_five.png");

	//John Nally: Load the walk animation frames for both players
	m_textures.Load(TextureID::kPlayer1Walk1, "Media/Textures/Player1_Walk1.png");
	m_textures.Load(TextureID::kPlayer1Walk2, "Media/Textures/Player1_Walk2.png");
	m_textures.Load(TextureID::kPlayer2Walk1, "Media/Textures/Player2_Walk1.png");
	m_textures.Load(TextureID::kPlayer2Walk2, "Media/Textures/Player2_Walk2.png");
}

void World::BuildScene()
{
	//Initialise the different layers
	for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); i++)
	{
		ReceiverCategories category = (i == static_cast<int>(SceneLayers::kAir)) ? ReceiverCategories::kScene : ReceiverCategories::kNone;
		SceneNode::Ptr layer(new SceneNode(category));
		m_scene_layers[i] = layer.get();
		m_scene_graph.AttachChild(std::move(layer));
	}

	//Prepare the background
	sf::Texture& texture = m_textures.Get(TextureID::kLandscape);
	sf::IntRect textureRect(m_world_bounds);
	texture.setRepeated(true);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition(sf::Vector2f(0, 0));
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Homework add the player's aircraft
	std::unique_ptr<Aircraft> leader(new Aircraft(AircraftType::kEagle, m_textures, m_fonts));
	m_player_aircraft = leader.get();
	m_player_aircraft->setPosition(m_spawn_position);
	m_player_aircraft->SetVelocity(0.f, 0.f);
	std::cout << "Player 1 spawn position: " << m_spawn_position.x << ", " << m_spawn_position.y << std::endl;

	m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(leader));

	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(soundNode));
	
	std::unique_ptr<Aircraft> player2(new Aircraft(AircraftType::kEagle2, m_textures, m_fonts));  //Ben Arrowsmith
	m_player_aircraft2 = player2.get();
	m_player_aircraft2->setPosition(m_spawn_position2);
	m_player_aircraft2->SetVelocity(0.f, 0.f);

	std::cout << "Player 2 spawn position: " << m_spawn_position2.x << ", " << m_spawn_position2.y << std::endl;
	m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(player2));
}

void World::AdaptPlayerVelocity()
{
	sf::Vector2f velocity = m_player_aircraft->GetVelocity();
	sf::Vector2f velocity2 = m_player_aircraft2->GetVelocity(); //Ben Arrowsmith

	//If they are moving diagonally divide by sqrt 2
	if (velocity.x != 0.f && velocity.y != 0.f)
	{
		m_player_aircraft->SetVelocity(velocity / std::sqrt(2.f));
	}
	if (velocity2.x != 0.f && velocity2.y != 0.f) //Ben Arrowsmith
	{
		m_player_aircraft2->SetVelocity(velocity2 / std::sqrt(2.f));
	}
}

void World::AdaptPlayerPosition()
{
	//keep player on the screen
	sf::FloatRect view_bounds(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
	const float border_distance = 40.f;

	sf::Vector2f position = m_player_aircraft->getPosition();
	position.x = std::min(position.x, view_bounds.size.x - border_distance);
	position.x = std::max(position.x, border_distance);
	position.y = std::min(position.y, view_bounds.position.y + view_bounds.size.y - border_distance);
	position.y = std::max(position.y, view_bounds.position.y + border_distance);
	m_player_aircraft->setPosition(position);

	sf::Vector2f position2 = m_player_aircraft2->getPosition(); //Ben Arrowsmith
	position2.x = std::min(position2.x, view_bounds.size.x - border_distance);
	position2.x = std::max(position2.x, border_distance);
	position2.y = std::min(position2.y, view_bounds.position.y + view_bounds.size.y - border_distance);
	position2.y = std::max(position2.y, view_bounds.position.y + border_distance);
	m_player_aircraft2->setPosition(position2);
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());;
}

sf::FloatRect World::GetBattleFieldBounds() const
{
	//Return camera bounds + a small area off screen where the enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.position.y -= 100.f;
	bounds.size.y += 100.f;
	return bounds;
}

bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();

	if ((static_cast<int>(type1) & category1) && (static_cast<int>(type2) & category2))
	{
		return true;
	}
	else if ((static_cast<int>(type1) & category2) && (static_cast<int>(type2) & category1))
	{
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}
}

void World::HandleCollisions()
{
	std::set<SceneNode::Pair> collision_pairs;
	m_scene_graph.CheckSceneCollision(m_scene_graph, collision_pairs);

	for (SceneNode::Pair pair : collision_pairs)
	{
		if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyProjectile))
		{
			//John Nally: WHEN PLAYER 1 SHOT, REMOVES 1 SCORE FROM PLAYER 1
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			aircraft.AddScore(-1);
			m_player1_score -= 1;
			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer2Aircraft, ReceiverCategories::kAlliedProjectile))
		{
			//John Nally: WHEN PLAYER 2 SHOT, REMOVES 1 SCORE FROM PLAYER 1
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			aircraft.AddScore(-1);
			m_player2_score -= 1;
			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kPointBox)) {
			//John Nally: PLAYER 1 COLLISION W/ POINTBOXES
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pointbox = static_cast<PointBox&>(*pair.second);

			int points = pointbox.GetPointValue();
			m_player1_score += points;
			player.AddScore(points);
			player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);

			pointbox.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayer2Aircraft, ReceiverCategories::kPointBox)) { 
			//John Nally: PLAYER 2 COLLISION W/ POINTBOXES
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pointbox = static_cast<PointBox&>(*pair.second);

			int points = pointbox.GetPointValue();
			m_player2_score += points;
			player.AddScore(points);
			player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);

			pointbox.Destroy();
		}
	}
}

void World::DestroyEntitiesOutsideView()
{
	Command command;
	command.category = static_cast<int>(ReceiverCategories::kProjectile) | static_cast<int>(ReceiverCategories::kPointBox);
	command.action = DerivedAction<Entity>([this](Entity& e, sf::Time dt)
	{
		//Does the object intersect with the battlefield
		if (GetBattleFieldBounds().findIntersection(e.GetBoundingRect()) == std::nullopt)
		{
			e.Destroy();
		}
	});
	m_command_queue.Push(command);
}

void World::UpdatePointBoxSpawning(sf::Time dt) { //John Nally: Timer for boxes spawning
	const sf::Time kSpawnInterval = sf::seconds(1.f); //Spawn every X Seconds (3 ATM)
	m_pointbox_spawn_timer += dt;

	if (m_pointbox_spawn_timer >= kSpawnInterval)
	{
		SpawnPointBoxes();
		m_pointbox_spawn_timer = sf::Time::Zero; //John Nally: Timer reset after summon
	}
}

//John Nally: Spawns a point box at a random X position within the view bounds, just above the top of the screen. The type of point box is also randomly selected.
void World::SpawnPointBoxes() {
	int random_type = Utility::RandomInt(static_cast<int>(PointBoxType::kPointBoxCount));
	PointBoxType type = static_cast<PointBoxType>(random_type);

	std::unique_ptr<PointBox> box(new PointBox(type, m_textures, m_fonts));

	sf::FloatRect view_bounds = GetViewBounds();

	float min_x = view_bounds.position.x + 50.f;
	float max_x = view_bounds.position.x + view_bounds.size.x - 50.f;
	int range = static_cast<int>(max_x - min_x);

	float spawn_x = min_x + static_cast<float>(Utility::RandomInt(range + 1));
	float spawn_y = view_bounds.position.y - 50.f;

	box->setPosition(sf::Vector2f(spawn_x, spawn_y));

	m_scene_layers[static_cast<int>(SceneLayers::kAir)]->AttachChild(std::move(box));

	//std::cout << "X Spawn: " << spawn_x << " Y Spawn: " << spawn_y << std::endl;
}
//John Nally: Getters for player scores to be used in GameState for UI display
int World::GetPlayer1Score() const {
	return m_player1_score;
}
//John Nally: Getters for player scores to be used in GameState for UI display
int World::GetPlayer2Score() const {
	return m_player2_score;
}

//John Nally: Compares the scores of both players and returns the winning player (1 or 2), or 0 if it's a tie or any other state
int World::GetWinningPlayer() const {
	if (m_player1_score > m_player2_score) {
		return 1; // Player 1 wins
	}
	else if (m_player2_score > m_player1_score) {
		return 2; // Player 2 wins
	}
	else {
		return 0; // Any other state or smthn IG
	}
}
//John Nally: Checks if either player has reached the score threshold to win the game (30 points in this case)
bool World::HasPlayerReachedPoints() const {
	return m_player_aircraft->GetScore() >= 30 || m_player_aircraft2->GetScore() >= 30;
}


void World::UpdateSounds() //Ben Arrowsmith
{
	sf::Vector2f listener_position;
	listener_position = m_camera.getCenter();
	m_sounds.SetListenerPosition(listener_position);
	m_sounds.RemoveStoppedSounds();
}
