#include "settings_state.hpp"
#include "Utility.hpp"
//John Nally D00258753
SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
    , m_gui_container()
    , m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
{
    AddButtonLabel(Action::kMoveUp, 80.f, 150.f, "P1 Move Up", context);
    AddButtonLabel(Action::kMoveDown, 80.f, 200.f, "P1 Move Down", context);
    AddButtonLabel(Action::kMoveRight, 80.f, 250.f, "P1 Move Right", context);
    AddButtonLabel(Action::kMoveLeft, 80.f, 300.f, "P1 Move Left", context);
    AddButtonLabel(Action::kBulletFire, 80.f, 350.f, "P1 Fire", context);

    AddButtonLabel(Action::kMoveUp, 320.f, 150.f, "P2 Move Up", context);
    AddButtonLabel(Action::kMoveDown, 320.f, 200.f, "P2 Move Down", context);
    AddButtonLabel(Action::kMoveRight, 320.f, 250.f, "P2 Move Right", context);
    AddButtonLabel(Action::kMoveLeft, 320.f, 300.f, "P2 Move Left", context);
    AddButtonLabel(Action::kBulletFire, 320.f, 350.f, "P2 Fire", context);
    

    UpdateLabels();

	auto back_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    back_button->setPosition(sf::Vector2f(80.f, 475.f));
    back_button->SetText("Back");
    back_button->SetCallback(std::bind(&SettingsState::RequestStackPop, this));
    m_gui_container.Pack(back_button);
}

void SettingsState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
}

bool SettingsState::Update(sf::Time dt)
{
    return true;
}

bool SettingsState::HandleEvent(const sf::Event& event)
{
    bool is_key_binding = false;

    //Iterate through all of the key binding buttons to see if they are being pressed, waiting for input from the user
    //Iterate through all of the key binding buttons to see if they are being pressed, waiting for input from the user
    for (std::size_t action = 0; action < static_cast<int>(Action::kActionCount); ++action)
    {
        if (m_binding_buttons[action]->IsActive())
        {
            is_key_binding = true;
            const auto* key_released = event.getIf<sf::Event::KeyReleased>();
            if (key_released)
            {
                GetContext().keys1->AssignKey(static_cast<Action>(action), key_released->scancode);
                m_binding_buttons[action]->Deactivate();
            }
            break;
        }
    }

        for (std::size_t action = 0; action < static_cast<int>(Action::kActionCount); ++action)
    {
        if (m_binding_buttons2[action]->IsActive())
        {
            is_key_binding = true;
            const auto* key_released = event.getIf<sf::Event::KeyReleased>();
            if (key_released)
            {
                GetContext().keys2->AssignKey(static_cast<Action>(action), key_released->scancode);
                m_binding_buttons2[action]->Deactivate();
            }
            break;
        }
    }

    if (is_key_binding)
    {
        UpdateLabels();
    }
    else
    {
        m_gui_container.HandleEvent(event);
    }
    return false;
}

void SettingsState::UpdateLabels()
{
    Player& player = *GetContext().player;
    for (std::size_t i = 0; i < static_cast<int>(Action::kActionCount); ++i)
    {
        sf::Keyboard::Scancode key = GetContext().keys1->GetAssignedKey(static_cast<Action>(i));
        m_binding_labels[i]->SetText(Utility::toString(key));
    }
}
//John Nally: Added x coordinate for buttons so player 2 buttons wouldnt be ontop of player 1
void SettingsState::AddButtonLabel(Action action, float x, float y, const std::string& text, Context context)
{
    m_binding_buttons[static_cast<int>(action)] = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_binding_buttons[static_cast<int>(action)]->setPosition(sf::Vector2f(x, y));
    m_binding_buttons[static_cast<int>(action)]->SetText(text);
    m_binding_buttons[static_cast<int>(action)]->SetToggle(true);

    m_binding_labels[static_cast<int>(action)] = std::make_shared<gui::Label>("",  * context.fonts);
    m_binding_labels[static_cast<int>(action)]->setPosition(sf::Vector2f(x + 300.f, y + 15.f));

    m_gui_container.Pack(m_binding_buttons[static_cast<int>(action)]);
    m_gui_container.Pack(m_binding_labels[static_cast<int>(action)]);
}
