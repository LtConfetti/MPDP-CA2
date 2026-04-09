#include "sound_node.hpp"

//Ben Arrowsmith D00257746
//Created Entire class from Johns code

SoundNode::SoundNode(SoundPlayer& player)
    : m_sounds(player)
{
}

void SoundNode::PlaySound(SoundEffect sound, sf::Vector2f position)
{
    m_sounds.Play(sound, position);
}

unsigned int SoundNode::GetCategory() const
{
    return static_cast<int>(ReceiverCategories::kSoundEffect);
}
