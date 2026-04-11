#include "menu_state.hpp"
#include "fontID.hpp"
#include <SFML/Graphics/Text.hpp>
#include "utility.hpp"
#include "menu_options.hpp"
#include "button.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

MenuState::MenuState(StateStack& stack, Context context) : State(stack, context), m_background_sprite(context.textures->Get(TextureID::kTitleScreen)), m_scoreboard_title(context.fonts->Get(FontID::kMain)), m_personal_wins(context.fonts->Get(FontID::kMain)), m_results_text(context.fonts->Get(FontID::kMain))
{
    auto play_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    play_button->setPosition(sf::Vector2f(384, 100));
    play_button->SetText("Play");
    play_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kGame);
        });

    auto host_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    host_button->setPosition(sf::Vector2f(384, 200));
    host_button->SetText("Host Game");
    host_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kHostGame);
        });

    auto join_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    join_button->setPosition(sf::Vector2f(384, 300));
    join_button->SetText("Join Game");
    join_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kJoinGame);
        });

    auto settings_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    settings_button->setPosition(sf::Vector2f(384, 400));
    settings_button->SetText("Settings");
    settings_button->SetCallback([this]()
        {
            RequestStackPush(StateID::kSettings);
        });

    auto exit_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    exit_button->setPosition(sf::Vector2f(384, 500));
    exit_button->SetText("Exit");
    exit_button->SetCallback([this]()
        {
            RequestStackPop();
        });

    m_gui_container.Pack(play_button);
    m_gui_container.Pack(host_button);
    m_gui_container.Pack(join_button);
    m_gui_container.Pack(settings_button);
    m_gui_container.Pack(exit_button);

	m_scoreboard_bg.setSize(sf::Vector2f(340.f, 600.f));
	m_scoreboard_bg.setPosition(sf::Vector2f(650.f, 80.f));
	m_scoreboard_bg.setFillColor(sf::Color(0, 0, 0, 160));

	int total_wins = 0;
    {
		std::ifstream stats("player_wins.txt");
		stats >> total_wins;
    }

	m_personal_wins.setString("Total Wins: " + std::to_string(total_wins));
	m_personal_wins.setPosition(sf::Vector2f(660.f, 100.f));
	m_personal_wins.setCharacterSize(18);

	m_scoreboard_title.setString("-- Match History --");
	m_scoreboard_title.setCharacterSize(20);
	m_scoreboard_title.setFillColor(sf::Color::Yellow);
	m_scoreboard_title.setPosition(sf::Vector2f(750.f, 140.f));

	m_results_text.setPosition(sf::Vector2f(750.f, 160.f));

	LoadResults();
}

void MenuState::LoadResults()
{
	std::ifstream stats("results.txt");

	std::vector<std::string> lines;
    std::string line;
    while (std::getline(stats, line))
    {
        if(!line.empty())
			lines.push_back(line);
    }
    
	const int kMax = 15;
    std::string combined;
	for (int i = 0; i < static_cast<int>(lines.size()) && i < kMax; ++i)
    {
        combined += lines[lines.size() - 1 - i] + "\n";
    }

	m_results_text.setString(combined);
}

void MenuState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_gui_container);

	window.draw(m_scoreboard_bg);
	window.draw(m_personal_wins);
	window.draw(m_scoreboard_title);
	window.draw(m_results_text);

}

bool MenuState::Update(sf::Time dt)
{
    return true;
}

bool MenuState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return true;
}

