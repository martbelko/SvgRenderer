#include "srpch.h"

#include "core/Window.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <iostream>

using namespace SvgRenderer;

int main(int argc, char** argv)
{
	Log::Init();
	SR_INFO("Initialized Log");

	Window wnd;
	bool running = true;

	while (running)
	{
		wnd.OnUpdate();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	return 0;
}
