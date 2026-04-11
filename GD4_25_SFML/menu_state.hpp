#pragma once
#include "state.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include "container.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <string>

class MenuState : public State
{
public:
	MenuState(StateStack& stack, Context context);
	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;
	void UpdateOptionText();

private:
	sf::Sprite m_background_sprite;
	gui::Container m_gui_container;

	void LoadResults();
	sf::RectangleShape m_scoreboard_bg;
	sf::Text m_scoreboard_title; 
	sf::Text m_personal_wins; //Personal Wins counted
	sf::Text m_results_text; //Match History e.g. "Game #1: Player 1 Won"

};

