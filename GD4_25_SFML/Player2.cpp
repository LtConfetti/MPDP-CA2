/*#include "player2.hpp"
#include "aircraft.hpp"

//Ben Arrowsmith D00257746

struct AircraftMover
{
    AircraftMover(float vx, float vy) : velocity2(vx, vy) {}
    void operator()(Aircraft& aircraft, sf::Time) const
    {
        aircraft.Accelerate(velocity2);
    }

    sf::Vector2f velocity2;
};

Player2::Player2()
{
    m_key_binding[sf::Keyboard::Scancode::J] = Action::kMoveLeft2;
    m_key_binding[sf::Keyboard::Scancode::L] = Action::kMoveRight2;
    m_key_binding[sf::Keyboard::Scancode::I] = Action::kMoveUp2;
    m_key_binding[sf::Keyboard::Scancode::K] = Action::kMoveDown2;
    m_key_binding[sf::Keyboard::Scancode::RShift] = Action::kBulletFire2;

    InitialiseActions();

    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayer2Aircraft);
    }
}


//Player::Player()
//{
//    m_key_binding[sf::Keyboard::Scancode::L] = Action::kMoveLeft;
//    m_key_binding[sf::Keyboard::Scancode::J] = Action::kMoveRight;
//    m_key_binding[sf::Keyboard::Scancode::I] = Action::kMoveUp;
//    m_key_binding[sf::Keyboard::Scancode::K] = Action::kMoveDown;
//    m_key_binding[sf::Keyboard::Scancode::N] = Action::kBulletFire;
//    m_key_binding[sf::Keyboard::Scancode::B] = Action::kMissileFire;
//
//    InitialiseActions();
//
//    for (auto& pair : m_action_binding)
//    {
//        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
//    }
//}

void Player2::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
   /* std::printf("test stuff");
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        auto found = m_key_binding.find(key_pressed->scancode);
        if (found != m_key_binding.end() && !IsRealTimeAction(found->second))
        {
            command_queue.Push(m_action_binding[found->second]);
        }
    }
}

void Player2::HandleRealTimeInput(CommandQueue& command_queue)
{
    for (auto pair : m_key_binding)
    {
        if (sf::Keyboard::isKeyPressed(pair.first) && IsRealTimeAction(pair.second))
        {
            command_queue.Push(m_action_binding[pair.second]);
        }
    }
}

void Player2::AssignKey(Action action, sf::Keyboard::Scancode key)
{
    //Remove keys that are currently bound to the action
    //Remove keys that are currently bound to the action
    for (auto itr = m_key_binding.begin(); itr != m_key_binding.end();)
    {
        if (itr->second == action)
        {
            m_key_binding.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }
    m_key_binding[key] = action;
}

sf::Keyboard::Scancode Player2::GetAssignedKey(Action action) const
{
    for (auto pair : m_key_binding)
    {
        if (pair.second == action)
        {
            return pair.first;
        }
    }
    return sf::Keyboard::Scancode::Unknown;
}

void Player2::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player2::GetMissionStatus() const
{
    return m_current_mission_status;
}

void Player2::InitialiseActions()
{
    const float kPlayerSpeed = 200.f;
    m_action_binding[Action::kMoveLeft2].action = DerivedAction<Aircraft>(AircraftMover(-kPlayerSpeed, 0.f));
    m_action_binding[Action::kMoveRight2].action = DerivedAction<Aircraft>(AircraftMover(kPlayerSpeed, 0.f));
    m_action_binding[Action::kMoveUp2].action = DerivedAction<Aircraft>(AircraftMover(0.f, -kPlayerSpeed));
    m_action_binding[Action::kMoveDown2].action = DerivedAction<Aircraft>(AircraftMover(0.f, kPlayerSpeed));
    m_action_binding[Action::kBulletFire2].action = DerivedAction<Aircraft>([](Aircraft& a, sf::Time dt)
        {
            a.Fire();
        }
    );

}

bool Player2::IsRealTimeAction(Action action)
{
    switch (action)
    {
    case Action::kMoveLeft2:
    case Action::kMoveRight2:
    case Action::kMoveUp2:
    case Action::kMoveDown2:
    case Action::kBulletFire2:
        return true;
    default:
        return false;
    }
}*/
