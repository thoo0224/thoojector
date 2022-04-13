#pragma once
#include <map>
#include <string>

#include "entry.h"

extern std::map<std::string, Process::ProcessEntry> g_ProcessEntries;

namespace Process
{

	void LoadProcessEntries();

}
