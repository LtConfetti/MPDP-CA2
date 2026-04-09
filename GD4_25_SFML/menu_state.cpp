#include "menu_state.hpp"
#include "fontID.hpp"
#include <SFML/Graphics/Text.hpp>
#include "utility.hpp"
#include "menu_options.hpp"
#include "button.hpp"

MenuState::MenuState(StateStack& stack, Context context) : State(stack, context), m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
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
}

void MenuState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
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

