#include "aircraft.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "utility.hpp"
#include "constants.hpp"
#include "projectile.hpp"
#include "projectile_type.hpp"
#include <iostream>
#include "sound_node.hpp"

//Ben Arrowsmith D00257746
//John Nally D00258753
namespace
{
	const std::vector<AircraftData> Table = InitializeAircraftData();
}

TextureID ToTextureID(AircraftType type)
{
	switch (type)
	{
	case AircraftType::kEagle:
		return TextureID::kEagle;
		break;
	case AircraftType::kEagle2:
		return TextureID::kEagle2;
		break;
	}
	return TextureID::kEagle;
}

Aircraft::Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts) 
	: Entity(Table[static_cast<int>(type)].m_hitpoints) 
	, m_type(type) 
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture))
	, m_health_display(nullptr)
	, m_fire_rate(1)
	, m_is_firing(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_is_marked_for_removal(false)
	, m_score_display(nullptr)
	, m_current_score(0)
	, m_textures(textures)
	, m_anim_timer(sf::Time::Zero)
	, m_anime_frame(0)
	, m_identifier(0) // default: unassigned (local-only mode)
{
	Utility::CentreOrigin(m_sprite);

	m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{		
			CreateBullet(node, textures);
		};

	std::string* health = new std::string("");
	std::unique_ptr<TextNode> health_display(new TextNode(fonts, *health));
	m_health_display = health_display.get();
	AttachChild(std::move(health_display));

	if (Aircraft::GetCategory() == static_cast<int>(ReceiverCategories::kPlayerAircraft))
	{
		//Collecting and adding Score
		std::string* score = new std::string("");
		std::unique_ptr<TextNode> score_display(new TextNode(fonts, *score));
		m_score_display = score_display.get();
		m_score_display->SetColor(sf::Color::Red); // Set score color to red for player 1
		AttachChild(std::move(score_display));
	}
	else if (Aircraft::GetCategory() == static_cast<int>(ReceiverCategories::kPlayer2Aircraft))
	{
		std::string* score = new std::string("");
		std::unique_ptr<TextNode> score_display(new TextNode(fonts, *score));
		m_score_display = score_display.get();
		m_score_display->SetColor(sf::Color(0, 100, 0)); // Set score color to red for player 1
		AttachChild(std::move(score_display));
	}
	UpdateTexts();
}

unsigned int Aircraft::GetCategory() const
{
	if (IsAllied())
	{
		return static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
	}
	else if (IsPlayer2()) //Ben Arrowsmith
	{
		return static_cast<unsigned int>(ReceiverCategories::kPlayer2Aircraft);
	}
	return static_cast<unsigned int>(ReceiverCategories::kEnemyAircraft);
}

void Aircraft::AddScore(int points)
{
	m_current_score = std::max(0, m_current_score += points); //stops from going below 0
	UpdateTexts();
}

void Aircraft::UpdateTexts()
{
	if (m_score_display)
	{
		m_score_display->setPosition(sf::Vector2f(0.f, 50.f));
		if (IsAllied()) {
			m_score_display->SetString("Score 1: " + std::to_string(m_current_score));
		}
		else if (IsPlayer2()) {
			m_score_display->SetString("Score 2: " + std::to_string(m_current_score));
		}
	}
}

float Aircraft::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

void Aircraft::Fire()
{
	if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_firing = true;
	}
}

void Aircraft::CreateBullet(SceneNode& node, const TextureHolder& textures) const
{
	ProjectileType type = IsAllied() ? ProjectileType::kAlliedBullet : ProjectileType::kEnemyBullet;
	CreateProjectile(node, type, 0.0f, 0.5f, textures);
}

void Aircraft::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures) const
{
	std::unique_ptr<Projectile> projectile(new Projectile(type, textures));
	sf::Vector2f offset(x_offset * m_sprite.getGlobalBounds().size.x, y_offset * m_sprite.getGlobalBounds().size.y);
	sf::Vector2f velocity(0, projectile->GetMaxSpeed());

	float sign = (IsAllied() || IsPlayer2()) ? -1.f: 1.f; //Ben Arrowsmith


	projectile->setPosition(GetWorldPosition() + offset * sign);
	projectile->SetVelocity(velocity * sign);
	node.AttachChild(std::move(projectile));
}

sf::FloatRect Aircraft::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Aircraft::IsMarkedForRemoval() const
{
	return m_is_marked_for_removal;
}

void Aircraft::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_sprite, states);
}

void Aircraft::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		m_is_marked_for_removal = true;

		SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2; //Ben Arrowsmith
		PlayLocalSound(commands, soundEffect);
		return;
	}
	Entity::UpdateCurrent(dt, commands);
	UpdateTexts();

	//Check if bullets were fired
	CheckProjectileLaunch(dt, commands);

	if (IsAllied() || IsPlayer2()) {
		UpdateAnimation(dt);
	}
}

void Aircraft::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{

	if (!IsAllied() && !IsPlayer2()) //Ben Arrowsmith
	{
		Fire();
	}

	if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
	{
		commands.Push(m_fire_command);
		m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval / (m_fire_rate + 1.f);
		m_is_firing = false;
		PlayLocalSound(commands, IsAllied() or IsPlayer2() ? SoundEffect::kEnemyGunfire : SoundEffect::kAlliedGunfire); //Ben Arrowsmith
	}
	else if (m_fire_countdown > sf::Time::Zero)
	{
		m_fire_countdown -= dt;
		m_is_firing = false;
	}
}

bool Aircraft::IsAllied() const
{
	return m_type == AircraftType::kEagle;
}

bool Aircraft::IsPlayer2() const //Ben Arrowsmith
{
	return m_type == AircraftType::kEagle2;
}


void Aircraft::PlayLocalSound(CommandQueue& commands, SoundEffect effect) //Ben Arrowsmith

{
	sf::Vector2f world_position = GetWorldPosition();
	Command command;
	command.category = static_cast<int>(ReceiverCategories::kSoundEffect);
	command.action = DerivedAction<SoundNode>(
		[effect, world_position](SoundNode& node, sf::Time)
		{
			node.PlaySound(effect, world_position);
		});
	commands.Push(command);
}

int Aircraft::GetScore() {
	return m_current_score;
}

void Aircraft::UpdateAnimation(sf::Time dt) {
	//John Nally: this function updates animation based on velocity, if moving, it cycles through 2 frames, else it goes to an idle frame
	//AI WAS USED IN THE CREATION OF THIS FUNCTION
	sf::Vector2f vel = GetVelocity();
	bool isMoving = (vel.x != 0.f || vel.y != 0.f);

	if (isMoving) {
		m_anim_timer += dt;
		if (m_anim_timer.asSeconds() >= kFrameTime) {
			m_anime_frame = (m_anime_frame + 1) % 2;
			m_anim_timer = sf::Time::Zero;

			TextureID frame;
			if (IsAllied()) {
				frame = (m_anime_frame == 0) ? TextureID::kPlayer1Walk1 : TextureID::kPlayer1Walk2;
			}
			else {
				frame = (m_anime_frame == 0) ? TextureID::kPlayer2Walk1 : TextureID::kPlayer2Walk2;
			}

			m_sprite.setTexture(m_textures.Get(frame));
			Utility::CentreOrigin(m_sprite);
		}
	}
	else {
		m_anim_timer = sf::Time::Zero;
		m_anime_frame = 0;
		TextureID defaultTex = IsAllied() ? TextureID::kEagle : TextureID::kEagle2;
		m_sprite.setTexture(m_textures.Get(defaultTex));
		Utility::CentreOrigin(m_sprite);
	}
}


//Network Identity Functions

uint8_t Aircraft::GetIdentifier() const
{
	return m_identifier;
}

void Aircraft::SetIdentifier(uint8_t identifier)
{
	m_identifier = identifier;
}