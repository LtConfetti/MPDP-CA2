#pragma once
//John Nally D00258753
namespace sf
{
	class RenderTarget;
	class RenderTexture;
	class Shader;
}

class PostEffect
{
public:
	virtual ~PostEffect();
	virtual void Apply(const sf::RenderTexture& input, sf::RenderTarget& output) = 0;
	static bool IsSupported();

protected:
	static void ApplyShader(const sf::Shader& shader, sf::RenderTarget& output);

};

