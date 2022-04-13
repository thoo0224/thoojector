#pragma once
#include <vector>
#include "imgui/imgui.h"

#include "panels/ProfilePanel.h"

extern std::vector<ProfilePanel> ProfilePanels;
extern ProfilePanel* g_CurrentProfilePanel;

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