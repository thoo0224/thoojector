#include "ProfilePanel.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <commdlg.h>
#include <tchar.h>

#include "../imgui/imgui.h"
#include "../injector.h"
#include "../utils.h"

using namespace Process;

void ProfilePanel::Render()
{
	ImVec2 AvailableRegion = ImGui::GetContentRegionAvail();

	ImGui::Text("Process");
	ImGui::Spacing();

	ImGui::RadioButton("Existing", &m_SelectedRadio, 0); ImGui::SameLine();
	ImGui::RadioButton("Launch", &m_SelectedRadio, 1); ImGui::SameLine();
	ImGui::RadioButton("Manual Launch", &m_SelectedRadio, 2);
	ImGui::Spacing();

	ImGui::SetNextItemWidth(AvailableRegion.x);
	if (ImGui::BeginCombo("##process_combo", m_SelectedProcessEntryLabel.data(), ImGuiComboFlags_HeightLarge))
	{
		for (auto& [key, Entry] : g_ProcessEntries)
		{
			bool IsSelected = Entry.szExeFile == m_SelectedProcessEntryLabel;
			if (ImGui::Selectable(key.data(), IsSelected))
				m_SelectedProcessEntryLabel = Entry.szExeFile;

			if (IsSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	ImGui::Spacing();
	ImGui::Text("Images");

	ImGuiTableFlags ImageTableFlags = ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders;
	if (ImGui::BeginTable("#images_table", 2, ImageTableFlags, ImVec2{ 0, 135 }))
	{

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Name");

		ImGui::TableNextColumn();
		ImGui::Text("Architecture");

		for (auto& Path : m_Images)
		{
			const std::string FileName = Path.filename().string();
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Selectable(FileName.c_str(), &m_SelectedImages[Path], ImGuiSelectableFlags_SpanAllColumns);

			// todo
			ImGui::TableNextColumn();
			ImGui::Text("x64");
		}

		ImGui::EndTable();
	}

	//if (ImGui::BeginDragDropTarget())
	//{
	//	if(const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("FILES", ImGuiDragDropFlags_SourceExtern))
	//	{
	//		printf("\n5p", Payload);
	//	}

	//	ImGui::BeginDragDropTarget();
	//}

	ImGui::Spacing();

	ImVec2 ButtonSize = ImVec2{ 75, 25 };
	if(ImGui::Button("Add", ButtonSize))
		OpenAddImageDialog();

	ImGui::SameLine();
	if(ImGui::Button("Remove", ButtonSize))
	{
		m_Images.clear();
		for(auto& [Image, IsSelected] : m_SelectedImages)
		{
			if (!IsSelected)
				continue;

			m_Images.push_back(Image);
		}

		m_SelectedImages.clear();
	}

	ImGui::SameLine();
	if(ImGui::Button("Clear", ButtonSize))
	{
		m_SelectedImages.clear();
		m_Images.clear();
	}

	AvailableRegion = ImGui::GetContentRegionAvail();
	AvailableRegion.x = 0;

	ImVec2 InjectButtonSize = ImVec2{ 80, 35 };
	float InjectButtonOffsetY = InjectButtonSize.y + 5;

	AvailableRegion.y -= InjectButtonOffsetY;
	ImGui::Dummy(AvailableRegion);

	if(ImGui::Button("Inject", InjectButtonSize))
		Inject();
}

// todo: duplicated files
void ProfilePanel::AddDroppedFiles(int PathNum, const char* Paths[])
{
	for(int i = 0; i < PathNum; i++)
	{
		const char* Path = Paths[i];
		if(!string_ends_with(Path, ".dll"))
		{
			printf("[WRN] Skipping file %s because it's not a DLL.\n", Path);
			continue;
		}

		m_Images.emplace_back(Path);
	}
}

void ProfilePanel::Inject()
{
	using namespace Injector;

	ProcessEntry& Entry = g_ProcessEntries[m_SelectedProcessEntryLabel];
	printf("[INF] Opening %s (%lu)\n", Entry.szExeFile.c_str(), Entry.pId);

	HANDLE pHandle = Entry.OpenHandle();
	if (pHandle == INVALID_HANDLE_VALUE)
	{
		printf("[ERR] Couldn't open handle to process\n");
		return;
	}

	printf("[INF] Successfully opened process\n");
	for(std::filesystem::path& ImagePath : m_Images)
	{
		std::string Copy = ImagePath.string();
		const char* szDllFile = Copy.c_str();
		if(!exists(ImagePath))
		{
			printf("[WRN] Failed to inject %s, the file doesn't exist anymore.\n", szDllFile);
			continue;
		}

		printf("[INF] Injecting %s\n", szDllFile);

		// todo multiple methods & error messages instead of codes
		InjectResult Result = NativeInject(pHandle, szDllFile);
		if(Result != InjectResult::Success)
		{
			printf("[ERR] Failed to inject, error code: 0x%x\n", Result);
			continue;
		}

		printf("[INF] Successfully injected!\n");
	}
}

void ProfilePanel::OpenAddImageDialog()
{
	GLFWwindow* Window = glfwGetCurrentContext();
	HWND hWnd = glfwGetWin32Window(Window);

	OPENFILENAME Ofn {};
	TCHAR szFile[260] {};

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.lpstrFile = szFile;
	Ofn.nMaxFile = sizeof szFile;
	Ofn.lpstrFilter = ".dll\0*.dll\0";
	Ofn.nFilterIndex = 1;
	Ofn.lpstrFileTitle = nullptr;
	Ofn.nMaxFileTitle = 0;
	Ofn.lpstrInitialDir = nullptr;
	Ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if(GetOpenFileNameA(&Ofn))
	{
		// todo: duplicated files
		m_Images.emplace_back(Ofn.lpstrFile);
	}
}
