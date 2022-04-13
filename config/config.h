#pragma once
#include <string>
#include <vector>
#include <filesystem>

#define CONFIG_FILENAME "settings.json"
#define CONFIG_PATH std::filesystem::current_path() / CONFIG_FILENAME

struct ConfigProfile
{
	std::string Name;
	std::vector<std::filesystem::path> Images;
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
