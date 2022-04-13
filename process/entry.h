#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Process
{

	class ProcessEntry
	{
	public:
		DWORD pId{};
		std::string szExeFile;

		HANDLE OpenHandle()
		{
			return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
		}
	};

}