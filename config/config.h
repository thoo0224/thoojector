#pragma once
#include <string>
#include <vector>

#define CONFIG_FILENAME "settings.json"
#define CONFIG_PATH std::filesystem::current_path() / CONFIG_FILENAME

struct ConfigProfile
{
	std::string Name;
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
