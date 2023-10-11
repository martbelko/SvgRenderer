#include "srpch.h"

#include "core/Application.h"

using namespace SvgRenderer;

int main(int argc, char** argv)
{
	Log::Init();
	SR_INFO("Initialized Log");

	Application app;
	app.Init();
	app.Run();
	app.Shutdown();

	return 0;
}
