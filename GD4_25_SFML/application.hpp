#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "player.hpp"
#include "resource_holder.hpp"
#include "resource_identifiers.hpp"
#include "statestack.hpp"
#include "key_binding.hpp"

//John Nally D00258753
//Difference Between CA1, references the new Key Binding class for local play between player 1 and 2, also all networked players use Player 1 Controls

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

	SoundPlayer m_sound;	//Ben Arrowsmith

	TextureHolder m_textures;
	FontHolder m_fonts;

	StateStack m_stack;
};

