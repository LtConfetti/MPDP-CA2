#pragma once
//John Nally D00258753
enum class MissionStatus
{
	kMissionRunning,
	kMissionSuccess,
	kMissionFailure,
	//John Nally: kPlayerXWins is the winstate for Player 1 or 2 winning
	kPlayer1Wins,
	kPlayer2Wins,
	kPlayerXWins
};