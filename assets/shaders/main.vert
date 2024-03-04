#version 460 core

layout(location = 0) uniform uvec2 res;
layout(location = 1) uniform uvec2 atlas_size;

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 col;

out vec2 v_uv;
out vec4 v_col;

void main()
{
	vec2 scaled = 2.0 * pos / vec2(res);
	gl_Position = vec4(scaled.x - 1.0, 1.0 - scaled.y, 0.0, 1.0);
	v_uv = uv / vec2(atlas_size);
	v_col = col;
}
