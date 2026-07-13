#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include <windows.h>
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "d3d11.lib")

namespace fs = std::filesystem;

// --- Глобальные переменные DirectX ---
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Логика лаунчера ---
enum class LauncherState { MAIN, SETTINGS, DOWNLOADING };
LauncherState currentState = LauncherState::MAIN;

int allocatedMemory = 4096;
float downloadProgress = 0.0f;
std::string statusText = "Waiting...";
char args[256] = "-cleanlasvrs";

void InstallMinecraftAndMod() {
    currentState = LauncherState::DOWNLOADING;
    
    downloadProgress = 0.1f;
    statusText = "Creating directories...";
    std::string basePath = "C:\\ArbuzProtect\\Reactive";
    fs::create_directories(basePath);
    Sleep(500); // Имитация задержки для красоты

    downloadProgress = 0.3f;
    statusText = "Downloading Fabric 1.21.4...";
    std::string fabricUrl = "https://maven.fabricmc.net/dir/fabric-installer/1.0.1/fabric-installer-1.0.1.jar";
    std::string fabricDest = basePath + "\\fabric-installer.jar";
    URLDownloadToFileA(NULL, fabricUrl.c_str(), fabricDest.c_str(), 0, NULL);

    downloadProgress = 0.6f;
    statusText = "Downloading custom mod...";
    std::string modUrl = "https://workupload.com/start/LG49qBadCk2";
    std::string modDest = basePath + "\\custom_mod.jar";
    URLDownloadToFileA(NULL, modUrl.c_str(), modDest.c_str(), 0, NULL);

    downloadProgress = 1.0f;
    statusText = "Done! Ready to launch.";
    Sleep(1000);
    
    // Возвращаемся в меню или запускаем игру
    currentState = LauncherState::MAIN; 
}

// --- Точка входа ---
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    // Создаем окно приложения
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ArbuzLauncher", nullptr };
    ::RegisterClassExW(&wc);
    
    // Окно фиксированного размера без возможности изменения
    int windowWidth = 400;
    int windowHeight = 550;
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ArbuzProtect Launcher", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2, (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2, 
        windowWidth, windowHeight, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Настройка Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; // Отключаем сохранение imgui.ini

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // --- Рендер интерфейса Лаунчера ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("LauncherUI", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        if (currentState == LauncherState::MAIN) {
            ImGui::Text("Versions");
            ImGui::Separator();
            
            ImGui::BeginChild("VersionBlock", ImVec2(0, 100), true);
            ImGui::Text("Minecraft 1.21.4");
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Beta");
            
            ImGui::SameLine(ImGui::GetWindowWidth() - 70);
            if (ImGui::Button("Play", ImVec2(50, 30))) {
                std::thread(InstallMinecraftAndMod).detach();
            }
            ImGui::EndChild();
        } 
        else if (currentState == LauncherState::SETTINGS) {
            if (ImGui::Button("<- Settings")) {
                currentState = LauncherState::MAIN;
            }
            ImGui::Separator();
            
            ImGui::Text("Java Settings");
            ImGui::SliderInt("Memory", &allocatedMemory, 1024, 8192, "%dMB");
            
            ImGui::Spacing();
            ImGui::Text("Minecraft Settings");
            ImGui::InputText("##args", args, IM_ARRAYSIZE(args));
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                currentState = LauncherState::MAIN;
            }
        }
        else if (currentState == LauncherState::DOWNLOADING) {
            ImGui::Text("Starting");
            ImGui::Separator();
            
            ImGui::BeginChild("DownloadBlock", ImVec2(0, 150), true);
            ImGui::Text("Reactive");
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "1.21.4 Beta");
            ImGui::Spacing();
            ImGui::TextWrapped("Take your Minecraft 1.21.4 experience to the next level. This client is loaded with features...");
            
            ImGui::Spacing();
            ImGui::Text("%s", statusText.c_str());
            ImGui::ProgressBar(downloadProgress, ImVec2(-1.0f, 0.0f));
            ImGui::EndChild();
        }

        // Нижняя панель
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 60);
        ImGui::Separator();
        
        ImGui::Text("al0n");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "2030.12.12");
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        if (ImGui::Button("Cfg", ImVec2(40, 30))) {
            currentState = LauncherState::SETTINGS;
        }
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 50);
        if (ImGui::Button("Exit", ImVec2(40, 30))) {
            done = true;
        }

        ImGui::End();

        // Отрисовка
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); 
    }

    // Очистка
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// --- Вспомогательные функции DirectX 11 ---
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}