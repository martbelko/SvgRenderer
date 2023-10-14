#version 460 core

layout (location = 0) in vec2 v_TexCoord;

layout (binding = 0) uniform sampler2D u_Texture;

layout (location = 0) out vec4 o_Color;

void main()
{
	vec4 s = texture(u_Texture, v_TexCoord);
	o_Color = s;
}