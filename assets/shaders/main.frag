#version 460 core

layout (binding = 0) uniform sampler2D tex;

in vec2 v_uv;
in vec4 v_col;

out vec4 f_col;

void main()
{
	f_col = v_col * vec4(1.0, 1.0, 1.0, texture(tex, v_uv).r);
}
