#pragma once
#include "texture_id.hpp"
#include "shader_types.hpp"
#include "resource_holder.hpp"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Shader.hpp>
#include "sound_effect.hpp"
#include "fontID.hpp"

namespace sf
{
	class Texture;
	class SoundBuffer; //Ben Arrowsmith
}

//template<typename Identifier, typename Resource>

typedef ResourceHolder<TextureID, sf::Texture> TextureHolder;
typedef ResourceHolder<FontID, sf::Font> FontHolder;
typedef ResourceHolder<SoundEffect, sf::SoundBuffer> SoundBufferHolder; //Ben Arrowsmith
typedef ResourceHolder<ShaderTypes, sf::Shader> ShaderHolder;

