#pragma once
#include "state.hpp"
#include <SFML/Graphics/Text.hpp>
//John Nally D00258753
//Difference Between CA1, added a new state for the end of the game, now displays a win or lose text for networked players.
class GameOverState : public State
{
public:
	GameOverState(StateStack& stack, Context context);
	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event);

private:
	sf::Text m_game_over_text;
	sf::Time m_elapsed_time;
	sf::Text m_winner_text;
	bool m_is_win;

};

