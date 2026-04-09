#include "game_state.hpp"
#include "mission_status.hpp"
#include <iostream>

//Ben Arrowsmith D00257746
//John Nally D00258753
GameState::GameState(StateStack& stack, Context context) : State(stack, context), m_world(*context.window, *context.sounds, *context.fonts), m_player(*context.player), m_player2(*context.player2) //Ben Arrowsmith
{

}

void GameState::Draw()
{
	m_world.Draw();
}

bool GameState::Update(sf::Time dt)
{
	m_world.Update(dt);
	//John Nally: Check for win condition based on whos win state is active, then pushes gameover state
	if (m_world.HasPlayerReachedPoints()) {
		int winner = m_world.GetWinningPlayer();

		if (winner == 1) {
			std::cout << "Player 1 Wins" << std::endl;
			m_player.SetMissionStatus(MissionStatus::kPlayer1Wins);

		}
		else if (winner == 2) {
			std::cout << "Player 2 Wins" << std::endl;
			m_player.SetMissionStatus(MissionStatus::kPlayer2Wins);
		}
		RequestStackPush(StateID::kGameOver);
 	}

	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleRealTimeInput(commands);
	m_player2.HandleRealTimeInput(commands); //Ben Arrowsmith
	return true;
}

bool GameState::HandleEvent(const sf::Event& event)
{
	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleEvent(event, commands);
	m_player2.HandleEvent(event, commands); //Ben Arrowsmith

	//Escape should bring up the pause menu
	//Escape should bring up the pause menu
	const auto* keypress = event.getIf<sf::Event::KeyPressed>();
	if(keypress && keypress->scancode == sf::Keyboard::Scancode::Escape)
	{
		RequestStackPush(StateID::kPause);
	}
	return true;
}


