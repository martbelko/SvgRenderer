#include "Log.h"

ref<Logger> Log::s_Logger;

void Log::Init()
{
	s_Logger = createRef<Logger>("LOG");
}

