#include "ImguiManager.h"
#include "imgui/imgui.h"


ImguiManager::ImguiManager()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\malgun.ttf", 24.0f, NULL, io.Fonts->GetGlyphRangesKorean());
}

ImguiManager::~ImguiManager()
{
    ImGui::DestroyContext();
}
