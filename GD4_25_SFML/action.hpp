#pragma once
//John Nally D00258753
//Difference Between CA1, removed the hardcoded player2, so now actions are synced across both players when playing local

enum class Action
{
	kMoveLeft,
	kMoveRight,
	kMoveUp,
	kMoveDown,
	kBulletFire,
	kActionCount
};