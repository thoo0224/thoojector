#pragma once
#include <string>
#include <vector>
#include <filesystem>

#include "../process/manager.h"

#define CONFIG_FILENAME "settings.json"
#define CONFIG_PATH std::filesystem::current_path() / CONFIG_FILENAME

struct ConfigProfile
{

	std::string Name;
	std::vector<std::filesystem::path> Images;
	std::string LastProcess;

	// todo: check for null or empty last process
	Process::ProcessEntry* GetLastProcessEntry()
	{
		for(auto& [_, Entry] : g_ProcessEntries)
		{
			if(!strcmp(Entry.szExeFile.c_str(), LastProcess.c_str()))
			{
				return &Entry;
			}
		}

		return nullptr;
	}

};

struct Config
{
	std::vector<ConfigProfile> Profiles;

	static Config Default()
	{
		Config Cfg{};
		Cfg.Profiles.emplace_back("Profile 1");

		return Cfg;
	}

};

extern Config g_Config;

namespace Configuration
{
	void ReadConfig();
	void SaveConfig();
}
