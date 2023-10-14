#version 460 core

layout (location = 0) in flat float v_Sgn;
layout (location = 1) in vec2 v_TexCoords;

layout (location = 0) out vec4 FragColor;

void main()
{
	float res = v_TexCoords.x * v_TexCoords.x - v_TexCoords.y;
	if (res < 0.0)
		FragColor = vec4(0, 1, 0, 1);
	else
		discard;
}
