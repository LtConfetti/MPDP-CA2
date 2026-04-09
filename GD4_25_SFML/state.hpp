#pragma once
#include <memory>
#include "resource_identifiers.hpp"
#include "player.hpp"
#include "player2.hpp" //Ben Arrowsmith
#include <SFML/Graphics/RenderWindow.hpp>
#include "stateid.hpp"
#include "sound_player.hpp" //Ben Arrowsmith

//Ben Arrowsmith D00257746

class StateStack;


class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	struct Context
	{ 
		Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, Player& player, Player2& player2, SoundPlayer& sound); //Ben Arrowsmith

		//TODO unique_ptr rather than raw pointers here?
		sf::RenderWindow* window;
		TextureHolder* textures;
		FontHolder* fonts;
		Player* player;
		Player2* player2; //Ben Arrowsmith
		SoundPlayer* sounds; //Ben Arrowsmith

	};

public:
	State(StateStack& stack, Context context);
	virtual ~State();
	virtual void Draw() = 0;
	virtual bool Update(sf::Time dt) = 0;
	virtual bool HandleEvent(const sf::Event& event) = 0;

protected:
	void RequestStackPush(StateID state_id);
	void RequestStackPop();
	void RequestStackClear();

	Context GetContext() const;

private:
	StateStack* m_stack;
	Context m_context;
};

