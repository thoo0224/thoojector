#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <map>

#include "../process/manager.h"

class ProfilePanel
{
public:
	const char* ProfileName;

	ProfilePanel(const char* ProfileName)
		: ProfileName(ProfileName)
	{
		// todo: make this better
		m_SelectedProcessEntryLabel = g_ProcessEntries.begin()->second.szExeFile;
	}

	void Render();
	void AddDroppedFiles(int PathNum, const char* Paths[]);

private:
	std::vector<std::filesystem::path> m_Images;
	std::vector<std::string> m_ProcessEntries;
	std::string m_SelectedProcessEntryLabel;

	std::map<std::filesystem::path, bool> m_SelectedImages;
	int m_SelectedRadio = 0;

	void Inject();
	void OpenAddImageDialog();
	void AddDllFile(std::filesystem::path&& Path);
};
