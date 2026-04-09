#pragma once
#include "texture_id.hpp"
#include <SFML/System/Time.hpp>
#include <functional>
#include "aircraft.hpp"
//John Nally D00258753

struct Direction
{
	Direction(float angle, float distance)
		: m_angle(angle), m_distance(distance) {
	}
	float m_angle;
	float m_distance;
};

struct AircraftData
{
	int m_hitpoints;
	float m_speed;
	TextureID m_texture;
	sf::Time m_fire_interval;
	std::vector<Direction> m_directions;
};

struct ProjectileData
{
	int m_damage;
	float m_speed;
	TextureID m_texture;
};

struct PointBoxData
{
	//John Nally: struct for Pointboxes, holds speed, point value (+/-), texture and movement pattern (directions)
	int m_point_value;
	float m_speed;
	TextureID m_texture;
	std::vector<Direction> m_directions;
};

std::vector<AircraftData> InitializeAircraftData();
std::vector<ProjectileData> InitializeProjectileData();
std::vector<PointBoxData> InitializePointBoxData();

