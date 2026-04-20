#pragma once
#include <memory>
#include "resource_identifiers.hpp"
#include "player.hpp"
#include "player2.hpp" 
#include <SFML/Graphics/RenderWindow.hpp>
#include "stateid.hpp"
#include "sound_player.hpp" 
#include "key_binding.hpp"

class StateStack;


class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	struct Context
	{ 
		Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, Player& player, SoundPlayer& sound, KeyBinding& keys1, KeyBinding& keys2);

		//TODO unique_ptr rather than raw pointers here?
		sf::RenderWindow* window;
		TextureHolder* textures;
		FontHolder* fonts;
		Player* player;
		SoundPlayer* sounds; 
		KeyBinding* keys1;    // P1 keys
		KeyBinding* keys2;    // P2 keys 

		Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, SoundPlayer& sound, KeyBinding& keys1, KeyBinding& keys2);

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

