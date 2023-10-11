#version 460 core

layout (location = 0) in vec3 a_Pos;

layout (location = 0) uniform mat4 u_ProjView;

void main()
{
	gl_Position = u_ProjView * vec4(a_Pos, 1.0);
}
