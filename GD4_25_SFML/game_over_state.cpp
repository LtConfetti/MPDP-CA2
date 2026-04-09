#include "game_over_state.hpp"
#include "utility.hpp"
#include "constants.hpp"
//John Nally D00258753
GameOverState::GameOverState(StateStack& stack, Context context)
    : State(stack, context)
    , m_game_over_text(context.fonts->Get(FontID::kMain))
    , m_elapsed_time(sf::Time::Zero)
	, m_winner_text(context.fonts->Get(FontID::kMain))
	, m_is_win(false)
{
    //John Nally: Game condition if Player 1 wins
    sf::Vector2f window_size(context.window->getSize());
    if (context.player->GetMissionStatus() == MissionStatus::kPlayer1Wins) {
        m_game_over_text.setString("Apple Jacks Wins!");
        m_game_over_text.setCharacterSize(50);
        m_game_over_text.setFillColor(sf::Color::Red);
		Utility::CentreOrigin(m_game_over_text);
        m_game_over_text.setPosition(sf::Vector2f(0.5 * window_size.x, 0.4 * window_size.y));
		m_is_win = true;
    }
	//John Nally: Game condition if Player 2 wins
    else if (context.player->GetMissionStatus() == MissionStatus::kPlayer2Wins) {
        m_game_over_text.setString("Jack's Apples Wins!");
        m_game_over_text.setCharacterSize(50);
        m_game_over_text.setFillColor(sf::Color::Green);
        Utility::CentreOrigin(m_game_over_text);
        m_game_over_text.setPosition(sf::Vector2f(0.5 * window_size.x, 0.4 * window_size.y));
        m_is_win = true;
    }
    else if (context.player->GetMissionStatus() == MissionStatus::kMissionSuccess)
    {
        //John Nally: Any other state, incase smthn goes wrong
        m_game_over_text.setString("Mission Success");
    }
    else
    {
        m_game_over_text.setString("Mission Failure");
    }

    m_game_over_text.setCharacterSize(70);
    Utility::CentreOrigin(m_game_over_text);
    m_game_over_text.setPosition(sf::Vector2f(0.5 * window_size.x, 0.4 * window_size.y));
}

void GameOverState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    //Create a dark semi-transparent background
    sf::RectangleShape background_shape;
    background_shape.setFillColor(sf::Color(0, 0, 0, 150));
    background_shape.setSize(window.getView().getSize());

    window.draw(background_shape);
    window.draw(m_game_over_text);
}

bool GameOverState::Update(sf::Time dt)
{
    //Show gameover for 3 seconds and then return to the main menu
    m_elapsed_time += dt;
    if (m_elapsed_time > sf::seconds(kGameOverToMenuPause))
    {
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return false;
}

bool GameOverState::HandleEvent(const sf::Event& event)
{
    return false;
}
