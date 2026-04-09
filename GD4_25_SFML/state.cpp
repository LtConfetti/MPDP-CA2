#include "state.hpp"
#include "statestack.hpp"

//Ben Arrowsmith D00257746

State::State(StateStack& stack, Context context) : m_stack(&stack), m_context(context)
{
}

State::~State()
{
}

State::Context::Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, Player& player, SoundPlayer& sound, KeyBinding& keys1, KeyBinding& keys2): window(&window), textures(&textures), fonts(&fonts), player(&player), sounds(&sound), keys1(&keys1), keys2(&keys2) //Ben Arrowsmith Player2 and sound
{
}

void State::RequestStackPush(StateID state_id)
{
	m_stack->PushState(state_id);
}

void State::RequestStackPop()
{
	m_stack->PopState();
}

void State::RequestStackClear()
{
	m_stack->ClearStack();
}

State::Context State::GetContext() const
{
	return m_context;
}
