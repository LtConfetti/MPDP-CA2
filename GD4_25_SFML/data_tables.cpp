#include "data_tables.hpp"
#include "aircraft_type.hpp"
#include "projectile_type.hpp"
#include "aircraft.hpp"
#include "constants.hpp"
#include "pointbox_type.hpp"

//Ben Arrowsmith D00257746
//John Nally D00258753
std::vector<AircraftData> InitializeAircraftData()
{
	std::vector<AircraftData> data(static_cast<int>(AircraftType::kAircraftCount));

	data[static_cast<int>(AircraftType::kEagle)].m_hitpoints = 100;
	data[static_cast<int>(AircraftType::kEagle)].m_speed = 200.f;
	data[static_cast<int>(AircraftType::kEagle)].m_fire_interval = sf::seconds(1);
	data[static_cast<int>(AircraftType::kEagle)].m_texture = TextureID::kEagle;

	data[static_cast<int>(AircraftType::kEagle2)].m_hitpoints = 100;//Ben Arrowsmith
	data[static_cast<int>(AircraftType::kEagle2)].m_speed = 200.f;//Ben Arrowsmith
	data[static_cast<int>(AircraftType::kEagle2)].m_fire_interval = sf::seconds(1);//Ben Arrowsmith
	data[static_cast<int>(AircraftType::kEagle2)].m_texture = TextureID::kEagle2;//Ben Arrowsmith

	return data;
}

std::vector<ProjectileData> InitializeProjectileData()
{
	std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));
	data[static_cast<int>(ProjectileType::kAlliedBullet)].m_damage = 10;
	data[static_cast<int>(ProjectileType::kAlliedBullet)].m_speed = 300;
	data[static_cast<int>(ProjectileType::kAlliedBullet)].m_texture = TextureID::kBullet;

	data[static_cast<int>(ProjectileType::kEnemyBullet)].m_damage = 10;
	data[static_cast<int>(ProjectileType::kEnemyBullet)].m_speed = 300;
	data[static_cast<int>(ProjectileType::kEnemyBullet)].m_texture = TextureID::kBullet;


	return data;
}

std::vector<PointBoxData> InitializePointBoxData()
{
	//John Nally:  Initalize PointBot structs with movement, point value and speeds given
	std::vector<PointBoxData> data(static_cast<int>(PointBoxType::kPointBoxCount));

	data[static_cast<int>(PointBoxType::kPlusOne)].m_point_value = 1;
	data[static_cast<int>(PointBoxType::kPlusOne)].m_speed = 150.f; //Slowest
	data[static_cast<int>(PointBoxType::kPlusOne)].m_texture = TextureID::kPointBoxPlusOne;
	data[static_cast<int>(PointBoxType::kPlusOne)].m_directions.emplace_back(Direction(+10.f, 50.f));
	data[static_cast<int>(PointBoxType::kPlusOne)].m_directions.emplace_back(Direction(-10.f, 100.f));
	data[static_cast<int>(PointBoxType::kPlusOne)].m_directions.emplace_back(Direction(+10.f, 50.f));

	data[static_cast<int>(PointBoxType::kPlusTwo)].m_point_value = 2;
	data[static_cast<int>(PointBoxType::kPlusTwo)].m_speed = 200.f; //Medium
	data[static_cast<int>(PointBoxType::kPlusTwo)].m_texture = TextureID::kPointBoxPlusTwo;
	data[static_cast<int>(PointBoxType::kPlusTwo)].m_directions.emplace_back(Direction(+20.f, 50.f));
	data[static_cast<int>(PointBoxType::kPlusTwo)].m_directions.emplace_back(Direction(+20.f, 100.f));
	data[static_cast<int>(PointBoxType::kPlusTwo)].m_directions.emplace_back(Direction(+20.f, 50.f));

	data[static_cast<int>(PointBoxType::kPlusThree)].m_point_value = 3;
	data[static_cast<int>(PointBoxType::kPlusThree)].m_speed = 400.f; //Fasstest
	data[static_cast<int>(PointBoxType::kPlusThree)].m_texture = TextureID::kPointBoxPlusThree;
	data[static_cast<int>(PointBoxType::kPlusThree)].m_directions.emplace_back(Direction(+50.f, 100.f));
	data[static_cast<int>(PointBoxType::kPlusThree)].m_directions.emplace_back(Direction(-50.f, 100.f));
	data[static_cast<int>(PointBoxType::kPlusThree)].m_directions.emplace_back(Direction(+50.f, 100.f));

	data[static_cast<int>(PointBoxType::kMinusFive)].m_point_value = -5;
	data[static_cast<int>(PointBoxType::kMinusFive)].m_speed = 250.f;
	data[static_cast<int>(PointBoxType::kMinusFive)].m_texture = TextureID::kPointBoxMinusFive;
	data[static_cast<int>(PointBoxType::kMinusFive)].m_directions.emplace_back(Direction(0.f, 0.f));
	data[static_cast<int>(PointBoxType::kMinusFive)].m_directions.emplace_back(Direction(0.f, 0.f));
	data[static_cast<int>(PointBoxType::kMinusFive)].m_directions.emplace_back(Direction(0.f, 0.f));

	return data;
}
