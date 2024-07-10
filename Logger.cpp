#include "Logger.h"

#include "DirectXCommon.h"

namespace Logger
{
	void Log(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
	}
}

