#ifndef GLSL_TEXTURE
#define GLSL_TEXTURE

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

#endif