#pragma once
#include <memory>
#include "resource_identifiers.hpp"
#include "player.hpp"
#include "player2.hpp" //Ben Arrowsmith
#include <SFML/Graphics/RenderWindow.hpp>
#include "stateid.hpp"
#include "sound_player.hpp" //Ben Arrowsmith
#include "key_binding.hpp"

//Ben Arrowsmith D00257746
//John Nally D00258753

class StateStack;


class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	struct Context
	{ 
		Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, Player& player, SoundPlayer& sound, KeyBinding& keys1, KeyBinding& keys2, uint8_t& winner_id);
		//Ben Arrowsmith

		//TODO unique_ptr rather than raw pointers here?
		sf::RenderWindow* window;
		TextureHolder* textures;
		FontHolder* fonts;
		Player* player;
		SoundPlayer* sounds; //Ben Arrowsmith
		KeyBinding* keys1;    // P1 keys
		KeyBinding* keys2;    // P2 keys 
		uint8_t winner_id = 0; //identify ID for winning player, default 0 for no winner

	};

public:
	State(StateStack& stack, Context context);
	virtual ~State();
	virtual void Draw() = 0;
	virtual bool Update(sf::Time dt) = 0;
	virtual bool HandleEvent(const sf::Event& event) = 0;

	virtual void OnActivate() {}
	virtual void OnDestroy() {}

protected:
	void RequestStackPush(StateID state_id);
	void RequestStackPop();
	void RequestStackClear();

	Context GetContext() const;

private:
	StateStack* m_stack;
	Context m_context;
};

