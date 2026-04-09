#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "player.hpp"
#include "resource_holder.hpp"
#include "resource_identifiers.hpp"
#include "statestack.hpp"
#include "key_binding.hpp"

//Ben Arrowsmith D00257746
class Application
{
public:
	Application();
	~Application();
	void Run();

private:
	void ProcessInput();
	void Update(sf::Time dt);
	void Render();
	void RegisterStates();

private:
	sf::RenderWindow m_window;
	KeyBinding m_key_binding_1;	// P1: WASD
	KeyBinding m_key_binding_2;	// P2: IJKL
	Player m_player;
	SoundPlayer m_sound;	//Ben Arrowsmith

	TextureHolder m_textures;
	FontHolder m_fonts;

	StateStack m_stack;
};

