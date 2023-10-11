#include "Filesystem.h"

namespace SvgRenderer {

	std::filesystem::path Filesystem::s_AssetsPath;

	void Filesystem::Init()
	{
	#ifdef _DEBUG
		s_AssetsPath = std::filesystem::current_path() / ".." / ".." / ".." / ".." / "assets";
	#endif
	}

}
