#include "srpch.h"

#include "core/Filesystem.h"
#include "core/Application.h"

#include <glm/glm.hpp>

using namespace SvgRenderer;

static bool IsValidChoice(const std::string& choice)
{
	if (choice.length() != 1)
	{
		return false;
	}

	if (choice[0] == '1' || choice[0] == '2' || choice[0] == '3')
	{
		return true;
	}

	return false;
}

int main(int argc, char** argv)
{
	Filesystem::Init();
	Log::Init();
	SR_INFO("Initialized Log");

	std::string choice = "";
	std::cout << "Select image to render:\n1) Tiger\n2) Paris\n3) World\n";

	while (true)
	{
		std::cin >> choice;
		if (!IsValidChoice(choice))
		{
			SR_INFO("Invalid choice. Try again");
		}
		else
		{
			break;
		}
	}

	auto GetFilename = [](char choice)
	{
		switch (choice)
		{
		case '1':
			return "tiger.svg";
		case '2':
			return "paris.svg";
		case '3':
			return "world.svg";
		}
	};

	const std::filesystem::path& svgPath = Filesystem::AssetsPath() / "svgs" / GetFilename(choice[0]);

	Application& app = Application::Get();
	app.Init(svgPath);
	app.Run();
	app.Shutdown();

	return 0;
}
