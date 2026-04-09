#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "player.hpp"
#include "resource_holder.hpp"
#include "resource_identifiers.hpp"
#include "statestack.hpp"

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
	Player* m_player;
	Player2* m_player2;		//Ben Arrowsmith
	SoundPlayer m_sound;	//Ben Arrowsmith

	TextureHolder m_textures;
	FontHolder m_fonts;

	StateStack m_stack;
};

