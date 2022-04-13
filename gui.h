#pragma once
#include <vector>
#include "imgui/imgui.h"

#include "panels/ProfilePanel.h"

extern ProfilePanel* g_CurrentProfilePanel;

namespace Gui
{

	using namespace Process;

	static std::vector<ProfilePanel> ProfilePanels;

	void Render();
	void Setup();
	void CreateNewProfile();
	void CloseProfile();
	void OpenProfile(ConfigProfile& Profile);

	void ApplyStyles();
}