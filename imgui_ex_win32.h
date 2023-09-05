#ifndef IMGUI_IMGUI_EX_WIN32_H_
#define IMGUI_IMGUI_EX_WIN32_H_

#include <imgui/imgui.h>
#include <imgui/imconfig.h>
#include <imgui/imgui_internal.h>
#include <imgui/imstb_rectpack.h>
#include <imgui/imstb_textedit.h>

#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include <d3d11.h>

void ImGuiInit();
void ImGuiUpdate();
void ImGuiExit();

namespace ImGuiEx {

void ExitApplication();
bool SetWindowTop(bool* top);
void SlowDown();

bool Begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
void End();

} // namespace ImGuiEx


#endif // IMGUI_IMGUI_EX_WIN32_H_