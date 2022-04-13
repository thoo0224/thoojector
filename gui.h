#pragma once
#include <vector>
#include "imgui/imgui.h"

#include "panels/ProfilePanel.h"

namespace Gui
{

	using namespace Process;

	static std::vector<ProfilePanel> ProfilePanels;

	void Render();
	void Setup();
	void ApplyStyles();

}