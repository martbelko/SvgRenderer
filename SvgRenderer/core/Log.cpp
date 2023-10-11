#include "Log.h"

namespace SvgRenderer {

	ref<Logger> Log::s_Logger;

	void Log::Init()
	{
		s_Logger = CreateRef<Logger>("LOG");
	}

}
