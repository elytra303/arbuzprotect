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
#include <windowsx.h>
#include <urlmon.h>

// Подключаем загрузчик картинок
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "d3d11.lib")

namespace fs = std::filesystem;

// --- D3D11 Глобальные переменные ---
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Текстуры для UI
ID3D11ShaderResourceView* iconTexture = nullptr;
ID3D11ShaderResourceView* bgTexture = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Функция загрузки JPG/PNG в DirectX 11 ---
bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
    int image_width = 0, image_height = 0, image_channels = 4;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &image_channels, 4);
    if (image_data == nullptr) return false;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = nullptr;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);
    return true;
}

// --- Логика Лаунчера ---
enum class LauncherState { MAIN, SETTINGS, DOWNLOADING };
LauncherState currentState = LauncherState::MAIN;

int allocatedMemory = 4096;
float downloadProgress = 0.0f;
std::string statusText = "Waiting...";
char args[256] = "-cleanlasvrs";

void InstallMinecraftAndMod() {
    currentState = LauncherState::DOWNLOADING;
    
    downloadProgress = 0.2f;
    statusText = "Creating directories...";
    std::string basePath = "C:\\ArbuzProtect\\Reactive";
    fs::create_directories(basePath);

    downloadProgress = 0.4f;
    statusText = "Downloading Fabric 1.21.4...";
    std::string fabricUrl = "https://maven.fabricmc.net/dir/fabric-installer/1.0.1/fabric-installer-1.0.1.jar";
    std::string fabricDest = basePath + "\\fabric-installer.jar";
    URLDownloadToFileA(NULL, fabricUrl.c_str(), fabricDest.c_str(), 0, NULL);

    downloadProgress = 0.7f;
    statusText = "Downloading mod...";
    std::string modUrl = "https://workupload.com/start/LG49qBadCk2";
    std::string modDest = basePath + "\\custom_mod.jar";
    URLDownloadToFileA(NULL, modUrl.c_str(), modDest.c_str(), 0, NULL);

    downloadProgress = 1.0f;
    statusText = "Done! Ready to launch.";
    Sleep(1000);
    currentState = LauncherState::MAIN; 
}

// --- Точка входа ---
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ArbuzLauncher", nullptr };
    ::RegisterClassExW(&wc);
    
    int windowWidth = 380;
    int windowHeight = 480;
    
    // WS_POPUP убирает стандартные белые рамки Windows 7/10
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ArbuzProtect", WS_POPUP, 
        (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2, (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2, 
        windowWidth, windowHeight, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; 

    // Загружаем системный шрифт (выглядит современно)
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f);
    if (!mainFont) io.Fonts->AddFontDefault();

    // Загрузка текстур
    int w, h;
    LoadTextureFromFile("icon.jpg", &iconTexture, &w, &h);
    LoadTextureFromFile("bg.jpg", &bgTexture, &w, &h);

    // --- СТИЛИЗАЦИЯ (В ТОЧНОСТИ КАК НА СКРИНЕ) ---
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(10, 10);
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.0f);
    colors[ImGuiCol_ChildBg]  = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_Button]   = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.95f, 1.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
        ImGui::Begin("LauncherUI", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // КАСТОМНАЯ ШАПКА
        ImGui::SetCursorPos(ImVec2(15, 15));
        
        if (currentState == LauncherState::MAIN) {
            ImGui::TextDisabled("Versions");
        } else if (currentState == LauncherState::SETTINGS) {
            if (ImGui::Button("<- Settings", ImVec2(100, 30))) currentState = LauncherState::MAIN;
        } else {
            ImGui::TextDisabled("Starting");
        }

        // Кнопки управления окном (Свернуть и Закрыть)
        ImGui::SetCursorPos(ImVec2(windowWidth - 70, 15));
        if (ImGui::Button("-", ImVec2(25, 25))) ShowWindow(hwnd, SW_MINIMIZE);
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(25, 25))) done = true;

        ImGui::SetCursorPos(ImVec2(15, 55)); // Отступ для контента

        if (currentState == LauncherState::MAIN) {
            // --- КАРТОЧКА ВЕРСИИ ---
            ImVec2 cardPos = ImGui::GetCursorScreenPos();
            ImVec2 cardSize = ImVec2(windowWidth - 30, 120);
            
            ImGui::BeginChild("VersionBlock", cardSize, false);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            // Отрисовка заблюренного фона
            if (bgTexture) {
                drawList->AddImageRounded(bgTexture, cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), ImVec2(0,0), ImVec2(1,1), IM_COL32_WHITE, 8.0f);
            } else {
                drawList->AddRectFilled(cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), IM_COL32(30, 30, 35, 255), 8.0f);
            }

            // Затемнение фона для читаемости
            drawList->AddRectFilled(cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), IM_COL32(0, 0, 0, 150), 8.0f);

            ImGui::SetCursorPos(ImVec2(15, 60));
            ImGui::Text("Minecraft 1.21.4");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Beta");
            
            ImGui::SetCursorPos(ImVec2(cardSize.x - 55, cardSize.y - 45));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.3f, 0.8f)); // Полупрозрачная кнопка
            if (ImGui::Button(">", ImVec2(40, 30))) { // Кнопка Play (стилизовал под треугольник)
                std::thread(InstallMinecraftAndMod).detach();
            }
            ImGui::PopStyleColor();
            ImGui::EndChild();
        } 
        else if (currentState == LauncherState::SETTINGS) {
            ImGui::BeginChild("SettingsBlock", ImVec2(windowWidth - 30, windowHeight - 120), false);
            ImGui::TextDisabled("Java Settings");
            ImGui::SliderInt("Memory", &allocatedMemory, 1024, 8192, "%dMB");
            
            ImGui::Spacing();
            ImGui::TextDisabled("Minecraft Settings");
            ImGui::InputText("##args", args, IM_ARRAYSIZE(args));
            if (ImGui::Button("Save", ImVec2(80, 30))) {
                currentState = LauncherState::MAIN;
            }
            ImGui::EndChild();
        }
        else if (currentState == LauncherState::DOWNLOADING) {
            ImGui::BeginChild("DownloadBlock", ImVec2(windowWidth - 30, 160), true);
            ImGui::Text("Reactive");
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "1.21.4 Beta");
            ImGui::Spacing();
            ImGui::TextWrapped("Take your Minecraft 1.21.4 experience to the next level. This client is loaded with features that give you a serious advantage, whether you're exploring, fighting, or building.");
            
            ImGui::SetCursorPosY(120);
            ImGui::TextDisabled("%s", statusText.c_str());
            ImGui::SameLine(windowWidth - 80);
            ImGui::Text("%d%%", (int)(downloadProgress * 100));
            
            ImGui::ProgressBar(downloadProgress, ImVec2(-1.0f, 4.0f), ""); // Тонкий прогресс-бар
            ImGui::EndChild();
        }

        // --- НИЖНЯЯ ПАНЕЛЬ С АВАТАРКОЙ И ДВЕРКОЙ ---
        ImGui::SetCursorPos(ImVec2(15, windowHeight - 65));
        ImVec2 avatarPos = ImGui::GetCursorScreenPos();
        
        // Рисуем аватарку
        if (iconTexture) {
            ImGui::GetWindowDrawList()->AddImageRounded(iconTexture, avatarPos, ImVec2(avatarPos.x + 40, avatarPos.y + 40), ImVec2(0,0), ImVec2(1,1), IM_COL32_WHITE, 12.0f);
        } else {
            ImGui::GetWindowDrawList()->AddRectFilled(avatarPos, ImVec2(avatarPos.x + 40, avatarPos.y + 40), IM_COL32(50, 50, 50, 255), 12.0f);
        }
        
        ImGui::SetCursorPos(ImVec2(65, windowHeight - 60));
        ImGui::BeginGroup();
        ImGui::Text("al0n");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "2030.12.12");
        ImGui::EndGroup();
        
        ImGui::SetCursorPos(ImVec2(windowWidth - 100, windowHeight - 55));
        if (ImGui::Button("Cfg", ImVec2(35, 35))) {
            currentState = LauncherState::SETTINGS;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Ext", ImVec2(35, 35))) {
            done = true;
        }

        ImGui::End();

        // Рендер
        ImGui::Render();
        const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); 
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// --- Обработчик окна (Для перетаскивания без рамок) ---
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
        case WM_NCHITTEST: { // Позволяет таскать окно за верхнюю часть
            LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
            if (hit == HTCLIENT) {
                POINT pt;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
                ScreenToClient(hWnd, &pt);
                if (pt.y < 40 && pt.x < 300) return HTCAPTION;
            }
            return hit;
        }
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Функции DirectX 11
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
