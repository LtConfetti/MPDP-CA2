#pragma once
//John Nally D00258753
//Difference Between CA1, added a category for the network to allow sending commands to all clients in the network
enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 << 0,
	kPlayerAircraft = 1 << 1,
	kAlliedAircraft = 1 << 2,
	kEnemyAircraft = 1 << 3, 
	kAlliedProjectile = 1 << 4,
	kEnemyProjectile = 1 << 5,
	kPickup = 1 << 6,
	kPlayer2Aircraft = 1 << 7, //Ben Arrowsmith
	kPointBox = 1 << 8,
	kSoundEffect = 1 << 9,
	kNetwork = 1 <<10,


	kAircraft = kPlayerAircraft | kAlliedAircraft | kEnemyAircraft,
	kProjectile = kAlliedProjectile | kEnemyProjectile
};

//A message that would be sent to all aircraft would be
//unsigned int all_aircraft = ReceiverCategories::kPlayer | ReceiverCategories::kAlloedAircraft | ReceiverCategories::kEnemyAircraft