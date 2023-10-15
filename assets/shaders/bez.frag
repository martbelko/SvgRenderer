#version 460 core

layout (location = 0) in vec2 v_TexCoords;

layout (location = 0) out vec4 FragColor;

void main()
{
	vec2 px = dFdx(v_TexCoords);
	vec2 py = dFdy(v_TexCoords);

	// Chain rule
	float fx = (2 * v_TexCoords.x) * px.x - px.y;
	float fy = (2 * v_TexCoords.x) * py.x - py.y;

	// Signed distance
	float sd = (v_TexCoords.x * v_TexCoords.x - v_TexCoords.y) / sqrt(fx * fx + fy * fy);

	// Linear alpha
	float alpha = 0.5 - sd;
	if (alpha > 1) // Inside
		alpha = 1;
	else if (alpha < 0)  // Outside
		discard;

	FragColor = vec4(0, 1, 0, alpha);
}
