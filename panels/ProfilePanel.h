#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <map>

#include "../process/manager.h"
#include "../config/config.h"

class ProfilePanel
{
public:
	ConfigProfile& Profile;
	std::string ProfileName;

	ProfilePanel(ConfigProfile& Profile)
		: Profile(Profile),
		  ProfileName(Profile.Name),
          m_Images(Profile.Images)
	{
		// todo: make this better
		m_SelectedProcessEntryLabel = g_ProcessEntries.begin()->second.GetFormatted();
	}

	void Render();
	void AddDroppedFiles(int PathNum, const char* Paths[]);

private:
	std::vector<std::filesystem::path>& m_Images;
	std::vector<std::string> m_ProcessEntries;
	std::string m_SelectedProcessEntryLabel;

	std::map<std::filesystem::path, bool> m_SelectedImages;
	int m_SelectedRadio = 0;

	void Inject();
	void OpenAddImageDialog();
	void AddDllFile(std::filesystem::path&& Path);
};
