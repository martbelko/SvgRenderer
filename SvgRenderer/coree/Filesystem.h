#pragma once

namespace SvgRenderer {

	class Filesystem
	{
	public:
		static void Init();

		static const std::filesystem::path& AssetsPath() { return s_AssetsPath; }
	private:
		static std::filesystem::path s_AssetsPath;
	};

}
