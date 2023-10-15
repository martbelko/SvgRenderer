#version 460 core

const vec2 tex_coords[3] = vec2[3] (
	vec2(0.0, 0.0),
	vec2(0.5, 0.0),
	vec2(1.0, 1.0)
);

layout (location = 0) in vec3 a_Pos;

layout (location = 0) uniform mat4 u_ProjView;

layout (location = 0) out vec2 v_TexCoords;

void main()
{
	v_TexCoords = tex_coords[gl_VertexID % 3];
	gl_Position = u_ProjView * vec4(a_Pos, 1.0);
}
