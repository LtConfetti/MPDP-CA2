#pragma once
#include "entity.hpp"
#include "aircraft_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "projectile_type.hpp"
#include "command_queue.hpp"

//Ben Arrowsmith D00257746
//John Nally D00258753

class Aircraft : public Entity
{
public:
	Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts);
	unsigned int GetCategory() const override;

	void AddScore(int points);
	int GetScore();

	void UpdateTexts();

	float GetMaxSpeed() const;
	void Fire();
	void CreateBullet(SceneNode& node, const TextureHolder& textures) const;
	void CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures) const;

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;

	void PlayLocalSound(CommandQueue& commands, SoundEffect effect); //Ben Arrowsmith


private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
	bool IsAllied() const;
	bool IsPlayer2() const; //Ben Arrowsmith

	void UpdateAnimation(sf::Time dt);

private:
	AircraftType m_type;
	sf::Sprite m_sprite;

	TextNode* m_health_display;
	TextNode* m_score_display; //John Nally: For Player UI to show score

	int m_current_score; //John Nally: current score saved

	Command m_fire_command;

	unsigned int m_fire_rate;

	bool m_is_firing;

	sf::Time m_fire_countdown;

	bool m_is_marked_for_removal;

	const TextureHolder& m_textures; //John Nally: reference for swapping between frames
	sf::Time m_anim_timer;
	int m_anime_frame; //John Nally:  current animation frame
	static constexpr float kFrameTime = 0.2f; //John Nally:  Time for each animation frame

};
