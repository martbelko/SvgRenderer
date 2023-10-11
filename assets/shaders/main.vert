#version 460 core

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec4 a_Color;

layout (location = 0) uniform mat4 u_ProjView;

layout (location = 0) out vec4 v_Color;

void main()
{
	v_Color = a_Color;
	gl_Position = u_ProjView * vec4(a_Pos, 0.0, 1.0);
}
