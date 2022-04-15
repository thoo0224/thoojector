#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <Windows.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>

#include "gui.h"
#include "utils.h"

#ifdef _DEBUG
#define USE_CONSOLE
#endif

#define WINDOW_HEIGHT 450
#define WINDOW_WIDTH 350
#define VSYNC 1

static ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;
static ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void RenderFrame(GLFWwindow* Window);

static void GlfwErrorCallback(int ErrorCode, const char* Description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", ErrorCode, Description);
}

static void GlfwDropCallback(GLFWwindow* Window, int PathNum, const char* Paths[])
{
	printf("[INF] Received glfwDropCallback with %d files.\n", PathNum);
	if (!g_CurrentProfilePanel)
	{
		printf("[WRN] Received file drop callback, but there is g_CurrentProfilePanel is null.\n");
		return;
	}

	g_CurrentProfilePanel->AddDroppedFiles(PathNum, Paths);
}

static void GifskiFrameBufferSizeCallback(GLFWwindow* Window, int Width, int Height)
{
	glViewport(0, 0, Width, Height);
	RenderFrame(Window);
}

void RenderFrame(GLFWwindow* Window)
{
	ImGuiIO& Io = ImGui::GetIO();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->WorkPos);
	ImGui::SetNextWindowSize(Viewport->WorkSize);
	ImGui::SetNextWindowViewport(Viewport->ID);

	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	if (DockspaceFlags & ImGuiConfigFlags_DockingEnable)
		WindowFlags |= ImGuiWindowFlags_NoBackground;

	ImGui::Begin("Thoojector", nullptr, WindowFlags);

	if (Io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID DockspaceId = ImGui::GetID("Dockspace");
		ImGui::DockSpace(DockspaceId, ImVec2(0.0f, 0.0f), DockspaceFlags);
	}

	Gui::Render();
	ImGui::End();
	ImGui::Render();

	int DisplayWidth = 0;
	int DisplayHeight = 0;
	glfwGetFramebufferSize(Window, &DisplayWidth, &DisplayHeight);
	glViewport(0, 0, DisplayWidth, DisplayHeight);\

	glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (Io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* BackupCurrentContext = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(BackupCurrentContext);
	}

	glfwSwapBuffers(Window);
}

int WinMain(
	_In_     HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPSTR     lpCmdLine,
	_In_     int       nShowCmd)
{
#ifdef USE_CONSOLE
	AllocConsole();
	FILE* pFile;
	freopen_s(&pFile, "CONOUT$", "w", stdout);
#endif

	//Configuration::ReadConfig(g_Config, CONFIG_PATH);
	Configuration::ReadConfig();

	glfwSetErrorCallback(GlfwErrorCallback);
	if (!glfwInit())
		return 1;

	constexpr size_t WindowTitleSize = 16;
	GLFWwindow* Window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, generate_random_string(WindowTitleSize).c_str(), nullptr, nullptr);
	if (!Window)
		return 1;

	glfwSetFramebufferSizeCallback(Window, GifskiFrameBufferSizeCallback);
	glfwSetDropCallback(Window, GlfwDropCallback);
	glfwMakeContextCurrent(Window);
	glfwSwapInterval(VSYNC);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& Io = ImGui::GetIO();
	Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	Io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	Io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\micross.ttf)", 15);

	Gui::ApplyStyles();

	ImGui_ImplGlfw_InitForOpenGL(Window, true);
	ImGui_ImplOpenGL3_Init();

	Process::LoadProcessEntries();
	std::thread ProcessScanningThread = std::thread([]
	{
		while(true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			while (true)
			{
				if (!g_IsProcessComboOpen)
					break;

				//printf("[INF] Waiting for combo box to close...\n");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			//printf("[INF] Scanning for processes\n");
			Process::LoadProcessEntries();

			bool FoundProcess = false;
			ProfilePanel* pPanel = g_CurrentProfilePanel;
			for(auto& [label, _] : g_ProcessEntries)
			{
				if(!strcmp(pPanel->m_SelectedProcessEntryLabel.c_str(), label.c_str()))
				{
					FoundProcess = true;
					break;
				}
			}

			if(!pPanel->m_SelectingClosedProcess && !FoundProcess)
			{
				pPanel->m_SelectingClosedProcess = true;
				std::string& Label = pPanel->m_SelectedProcessEntryLabel;
				Label = std::format("{} (closed)", Label);
			}
		}
	});

	Gui::Setup();

	while(!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
		RenderFrame(Window);
	}

	ProcessScanningThread.detach();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(Window);
	glfwTerminate();

	Configuration::SaveConfig();
	return 0;
}