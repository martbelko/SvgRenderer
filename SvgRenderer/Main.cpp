#include "srpch.h"

#include "core/Filesystem.h"
#include "core/Application.h"

using namespace SvgRenderer;

int main(int argc, char** argv)
{
	Filesystem::Init();
	Log::Init();
	SR_INFO("Initialized Log");

	Application& app = Application::Get();
	app.Init();
	app.Run();
	app.Shutdown();

	return 0;
}
