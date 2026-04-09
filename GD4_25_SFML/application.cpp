#include "application.hpp"
#include "constants.hpp"
#include "fontid.hpp"
#include "game_state.hpp"
#include "title_state.hpp"
#include "menu_state.hpp"
#include "pause_state.hpp"
#include "settings_state.hpp"
#include "game_over_state.hpp"
#include "state.hpp"
#include "multiplayer_gamestate.hpp"
//John Nally D00258753
Application::Application() : m_window(sf::VideoMode({ 1024, 768 }), "States", sf::Style::Close),
m_key_binding_1(1), 
m_key_binding_2(2),
m_player(nullptr, 1, &m_key_binding_1), 

m_stack(State::Context(m_window, m_textures, m_fonts, m_player, m_sound, m_key_binding_1, m_key_binding_2))
{
	m_window.setKeyRepeatEnabled(false);
	m_fonts.Load(FontID::kMain, "Media/Fonts/Sansation.ttf");
	m_textures.Load(TextureID::kEagle, "Media/Textures/Player1.png");
	m_textures.Load(TextureID::kEagle2, "Media/Textures/Player2.png"); //Ben Arrowsmith
	m_textures.Load(TextureID::kPlayer1Walk1, "Media/Textures/Player1_Walk1.png");
	m_textures.Load(TextureID::kPlayer1Walk2, "Media/Textures/Player1_Walk2.png");
	m_textures.Load(TextureID::kPlayer2Walk1, "Media/Textures/Player2_Walk1.png");
	m_textures.Load(TextureID::kPlayer2Walk2, "Media/Textures/Player2_Walk2.png");
	m_textures.Load(TextureID::kTitleScreen, "Media/Textures/TitleScreen.png");
	m_textures.Load(TextureID::kButtonNormal, "Media/Textures/ButtonNormal.png");
	m_textures.Load(TextureID::kButtonSelected, "Media/Textures/ButtonSelected.png");
	m_textures.Load(TextureID::kButtonActivated, "Media/Textures/ButtonPressed.png");
	m_textures.Load(TextureID::kPointBoxPlusOne, "Media/Textures/box_plus_one.png");
	m_textures.Load(TextureID::kPointBoxPlusTwo, "Media/Textures/box_plus_two.png");
	m_textures.Load(TextureID::kPointBoxPlusThree, "Media/Textures/box_plus_three.png");

	RegisterStates();
	m_stack.PushState(StateID::kTitle);
}

Application::~Application() {

}

void Application::Run()
{
	sf::Clock clock;
	sf::Time time_since_last_update = sf::Time::Zero;
	while (m_window.isOpen())
	{
		time_since_last_update += clock.restart();
		while (time_since_last_update.asSeconds() > kTimePerFrame)
		{
			time_since_last_update -= sf::seconds(kTimePerFrame);
			ProcessInput();
			Update(sf::seconds(kTimePerFrame));

			if (m_stack.IsEmpty())
			{
				m_window.close();
			}
		}
		Render();
	}
}

void Application::ProcessInput()
{
	while (const std::optional event = m_window.pollEvent())
	{
		m_stack.HandleEvent(*event);

		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
		}

	}
}

void Application::Update(sf::Time dt)
{
	m_stack.Update(dt);
}

void Application::Render()
{
	m_window.clear();
	m_stack.Draw();
	m_window.display();
}

void Application::RegisterStates()
{
	m_stack.RegisterState<TitleState>(StateID::kTitle);
	m_stack.RegisterState<MenuState>(StateID::kMenu);
	m_stack.RegisterState<GameState>(StateID::kGame);
	m_stack.RegisterState<PauseState>(StateID::kPause);
	m_stack.RegisterState<SettingsState>(StateID::kSettings);
	m_stack.RegisterState<GameOverState>(StateID::kGameOver);

	m_stack.RegisterState<MultiplayerGameState>(StateID::kHostGame, true);
	m_stack.RegisterState<MultiplayerGameState>(StateID::kJoinGame, false);
	m_stack.RegisterState<PauseState>(StateID::kNetworkPause);
}


