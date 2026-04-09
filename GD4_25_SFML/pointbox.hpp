#pragma once
#include "entity.hpp"
#include "pointbox_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "command_queue.hpp"

class PointBox : public Entity
{
	public:
	PointBox(PointBoxType type, const TextureHolder& textures, const FontHolder& fonts);
	unsigned int GetCategory() const override;
	void UpdateTexts();
	void UpdateMovementPattern(sf::Time dt);
	float GetMaxSpeed() const;
	int GetPointValue() const;
	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

private:
	PointBoxType m_type;
	sf::Sprite m_sprite;
	TextNode* m_point_display;
	float m_distance_travelled;
	int m_directions_index;
	bool m_is_marked_for_removal;
};