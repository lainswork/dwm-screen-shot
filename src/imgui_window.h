#pragma once
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
namespace imgui_window {
	bool init();
	void destroy();
	bool begin();
	void end();
	ImVec2 GetGuiWindowSize();
}