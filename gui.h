#pragma once
#include <vector>
#include "imgui/imgui.h"

#include "panels/ProfilePanel.h"

extern std::vector<ProfilePanel> g_ProfilePanels;
extern ProfilePanel* g_CurrentProfilePanel;
extern bool g_IsProcessComboOpen;

namespace Gui
{

	using namespace Process;

	void Render();
	void Setup();
	void CreateNewProfile();
	void CloseProfile();
	void OpenProfile(ConfigProfile& Profile);

	void ApplyStyles();
}