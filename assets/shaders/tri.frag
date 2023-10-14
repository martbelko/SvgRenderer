#version 460 core

layout (location = 0) in flat float v_Sgn;

layout (location = 0) out vec4 FragColor;

void main()
{
	if (v_Sgn > 0.0)
		FragColor = vec4(0, 1, 0, 1);
	else
		discard;
}
