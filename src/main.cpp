#include "resource.h"
#include <Windows.h>

#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif



// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui_window.h"
#include "dwm_symbol.h"
#include "win-url-download.hpp"
#include <thread>
#include <atomic>
#define PrintErro(text) MessageBoxA((HWND)0,text ,"Erro",MB_OK |MB_TOPMOST)


void open_binary_file(const std::string& file, std::vector<uint8_t>& data);
void buffer_to_file_bin(unsigned char* buffer, size_t buffer_size, const std::string& filename);

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    std::atomic<float> DownloadProgress;
    std::atomic<bool> DownloadErro;


    std::string pdb_url = dwm_symbol::pdburl(dwm_symbol::GetModuleDebugInfo("d3d11.dll"));


    if (!imgui_window::init()) {
        return -1;
    }

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (imgui_window::begin()) {

            ImGui::SetNextWindowPos({ 0,0 }, ImGuiCond_Always);
            ImGui::SetNextWindowSize(imgui_window::GetGuiWindowSize(), ImGuiCond_Always);
            ImGui::Begin(u8"DWM.EXE 截图", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
            
            static bool show_text = false;
            if (show_text) {
                
                ImGui::Text(u8"符号url:  %s", pdb_url.c_str());
                ImGui::Text(u8"下载进度:  %f", DownloadProgress.load());
                ImGui::Text(u8"当前状态:  %s", DownloadErro.load() ? u8"下载失败" : DownloadProgress.load() == 100.f ? dwm_symbol::hook_offset ? u8"偏移获取完毕" : u8"分析符号中" : u8"正在下载");
                ImGui::Text(u8"偏移 0x%x", dwm_symbol::hook_offset);
            }
            else
            {
                if (ImGui::Button(u8"点击初始化")) {
                    show_text = true;
                    std::thread([=, &DownloadProgress, &DownloadErro]() {
                        CBindStatusCallback* StatusCallback = CBindStatusCallback::GenerateAnInstance();
                        StatusCallback->OnProgressCallBack([=, &DownloadProgress, &DownloadErro](float Progress) { DownloadProgress = Progress;
                            });

                        auto hr = URLDownloadToFileA(0, pdb_url.c_str(), "d3d11.pdb", 0, StatusCallback);
                        if (hr == S_OK) {
                            std::vector<uint8_t> data;
                            open_binary_file("d3d11.pdb", data);
                            if (!dwm_symbol::init(data.data())) {
                                PrintErro("获取符号失败!");
                            }
                        }
                        else {
                            DownloadErro = true;
                        }
                        StatusCallback->Release();
                        }).detach();
                }
            }


           
            if (dwm_symbol::hook_offset) {
                if (ImGui::Button(u8"点击测试截图")) {
                
                
                }
            }


            ImGui::End();

            imgui_window::end();
        }
        
    }

    imgui_window::destroy();
    return 0;
}

// Helper functions

void open_binary_file(const std::string& file, std::vector<uint8_t>& data)
{
    std::ifstream fstr(file, std::ios::binary);
    fstr.unsetf(std::ios::skipws);
    fstr.seekg(0, std::ios::end);

    const auto file_size = fstr.tellg();

    fstr.seekg(NULL, std::ios::beg);
    data.reserve(static_cast<uint32_t>(file_size));
    data.insert(data.begin(), std::istream_iterator<uint8_t>(fstr), std::istream_iterator<uint8_t>());
}

void buffer_to_file_bin(unsigned char* buffer, size_t buffer_size, const std::string& filename) {
    std::ofstream file(filename, std::ios_base::out | std::ios_base::binary);
    file.write((const char*)buffer, buffer_size);
    file.close();
}