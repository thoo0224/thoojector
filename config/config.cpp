#include "config.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace nlohmann;

Config g_Config = Config::Default();

namespace Configuration
{
	void ReadConfig()
	{
		if (!exists(CONFIG_PATH))
			return;

		std::ifstream File(CONFIG_PATH);
		json Json;
		File >> Json;

		json ProfileArray = Json["Profiles"];

		size_t NumProfiles = ProfileArray.size();
		std::vector<ConfigProfile> Profiles;
		for(size_t i = 0; i < NumProfiles; i++)
		{
			ConfigProfile Profile {};
			json ProfileJson = ProfileArray.at(i);

			Profile.Name = ProfileJson["Name"];
			Profiles.emplace_back(Profile);
		}

		g_Config.Profiles = Profiles;
	}

	void SaveConfig()
	{
		json Json;
		json ProfileArray = json::array();
		for (ConfigProfile& Profile : g_Config.Profiles)
		{
			json ProfileJson;
			ProfileJson["Name"] = Profile.Name;

			ProfileArray.emplace_back(ProfileJson);
		}

		Json["Profiles"] = ProfileArray;

		std::string Content = Json.dump(4);
		std::ofstream File(CONFIG_PATH);
		File.write(Content.c_str(), sizeof(char) * Content.size());
		File.close();
	}

}