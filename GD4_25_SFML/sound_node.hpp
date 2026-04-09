#pragma once
#include "scene_node.hpp"
#include "sound_player.hpp"

//Ben Arrowsmith D00257746
//Created Entire header from Johns code

class SoundNode : public SceneNode
{
public:
	SoundNode(SoundPlayer& player);
	void PlaySound(SoundEffect sound, sf::Vector2f position);

	virtual unsigned int GetCategory() const override;

private:
	SoundPlayer& m_sounds;
};

