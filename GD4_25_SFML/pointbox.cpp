#include "pointbox.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "utility.hpp"
#include <iostream>
//John Nally D00258753
namespace {
	const std::vector<PointBoxData> Table = InitializePointBoxData();
}

TextureID ToTextureID(PointBoxType type)
{
	switch (type)
	{
	case PointBoxType::kPlusOne:
		return TextureID::kPointBoxPlusOne;
		break;
	case PointBoxType::kPlusTwo:
		return TextureID::kPointBoxPlusTwo;
		break;
	case PointBoxType::kPlusThree:
		return TextureID::kPointBoxPlusThree;
		break;
	case PointBoxType::kMinusFive:
		return TextureID::kPointBoxMinusFive;
		break;
	}

	return TextureID::kPointBoxPlusOne;
}

PointBox::PointBox(PointBoxType type, const TextureHolder& textures, const FontHolder& fonts) : Entity(1)
, m_type(type)
, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture))
, m_point_display(nullptr)
, m_distance_travelled(0.f)
, m_directions_index(0)
, m_is_marked_for_removal(false)
{
	Utility::CentreOrigin(m_sprite);

	std::string* point_text = new std::string("+" + std::to_string(GetPointValue()));
	std::unique_ptr<TextNode> point_display(new TextNode(fonts, *point_text));
	m_point_display = point_display.get();
	m_point_display->setPosition(sf::Vector2f(0.f, 32.f));
	m_point_display->SetColor(sf::Color(0,158,96));
	AttachChild(std::move(point_display));
	if (GetPointValue() < 0) {
		m_point_display->SetString(std::to_string(GetPointValue()));
		m_point_display->SetColor(sf::Color::Red);
		m_point_display->setPosition(sf::Vector2f(0.f, 42.f));
	}

	UpdateTexts();
}

unsigned int PointBox::GetCategory() const
{
	return static_cast<unsigned int>(ReceiverCategories::kPointBox);
}

void PointBox::UpdateTexts()
{
	if (m_point_display)
	{
		m_point_display->setRotation(-getRotation());
	}
}

void PointBox::UpdateMovementPattern(sf::Time dt)
{
	const std::vector<Direction>& directions = Table[static_cast<int>(m_type)].m_directions;
	if (!directions.empty())
	{
		//Move along the current direction for distance and then change direction
		if (m_distance_travelled > directions[m_directions_index].m_distance)
		{
			m_directions_index = (m_directions_index + 1) % directions.size();
			m_distance_travelled = 0;
		}

		//Compute the velocity
		//Add 90 to move down the screen, 0 degrees is to the right
		double radians = Utility::toRadians(directions[m_directions_index].m_angle + 90.f);
		float vx = GetMaxSpeed() * std::cos(radians);
		float vy = GetMaxSpeed() * std::sin(radians);

		SetVelocity(sf::Vector2f(vx, vy));
		m_distance_travelled += GetMaxSpeed() * dt.asSeconds();
	}
}

float PointBox::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

int PointBox::GetPointValue() const
{
	return Table[static_cast<int>(m_type)].m_point_value;
}

sf::FloatRect PointBox::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool PointBox::IsMarkedForRemoval() const
{
	return m_is_marked_for_removal;
}

void PointBox::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_sprite, states);
}

void PointBox::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		m_is_marked_for_removal = true;
		return;
	}

	Entity::UpdateCurrent(dt, commands);
	UpdateTexts();
	UpdateMovementPattern(dt);
}