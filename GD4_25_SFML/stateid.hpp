#pragma once

//John Nally D00258753
//Added states for join game, host game and network pause for networked games
enum class StateID
{
	kNone,
	kTitle,
	kMenu,
	kGame,
	kPause,
	kNetworkPause,
	kSettings,
	kGameOver,
	kHostGame, 
	kJoinGame,
};