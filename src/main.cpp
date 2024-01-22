#include <d3d11.h>
#include <tchar.h>
#include "libaries/imgui/imgui.h"
#include "libaries/imgui/imgui_internal.h"
#include "libaries/imgui/backends/imgui_impl_dx11.h"
#include "libaries/imgui/backends/imgui_impl_win32.h"
#include "utils/ImGuiEx.h"
#include <winuser.h>
#include <windows.h>

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Main window
HWND g_mainWindow = nullptr;

// Overlay state
bool g_overlayEnabled = false;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RenderOverlay();
void SetOverlayTransparent(HWND hWnd);
void SetOverlayAlwaysOnTop(HWND hWnd, bool alwaysOnTop);
void SetOverlayAlwaysOnTopFullscreenOptimized(HWND hWnd);
bool IsFullscreenWindow(HWND hWnd);

bool IsFullscreenWindow(HWND hWnd)
{
    // Check if the window style includes WS_POPUP and does not include WS_CHILD
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    bool isFullscreenStyle = (style & WS_POPUP) && !(style & WS_CHILD);

    // Check if the window is not minimized
    bool isNotMinimized = IsIconic(hWnd) == 0;

    // Check if the window dimensions match the screen dimensions
    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    bool isMatchingScreenDimensions = (windowWidth == screenWidth) && (windowHeight == screenHeight);

    // Determine if it's a fullscreen window based on the combined checks
    return isFullscreenStyle && isNotMinimized && isMatchingScreenDimensions;
}
// Main code
int main(int, char**)
{
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        CreateSolidBrush(RGB(0, 0, 0)), // set background color
        nullptr,
        L"OverlayWindow",
        nullptr
    };
    ::RegisterClassExW(&wc);

    g_mainWindow = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        wc.lpszClassName,
        L"My Transparent Overlay",
        WS_POPUP,
        1,
        1,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    if (!CreateDeviceD3D(g_mainWindow))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    SetOverlayTransparent(g_mainWindow);
    g_ResizeWidth = GetSystemMetrics(SM_CXSCREEN);
    g_ResizeHeight = GetSystemMetrics(SM_CYSCREEN);

    ShowWindow(g_mainWindow, SW_SHOWDEFAULT);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(g_mainWindow);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    //bool done = false;
    //while (!done)
    //{
    //    MSG msg;
    //    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    //    {
    //        ::TranslateMessage(&msg);
    //        ::DispatchMessage(&msg);
    //        if (msg.message == WM_QUIT)
    //            done = true;
    //    }

    //    if (done)
    //        break;
    //    Sleep(10);

    //    HWND foregroundWindow = GetForegroundWindow();
    //    bool isFullscreen = IsFullscreenWindow(foregroundWindow);

    //    if ((isFullscreen && !g_overlayEnabled) || (!isFullscreen && g_overlayEnabled))
    //    {
    //        if (isFullscreen)
    //            SetOverlayAlwaysOnTopFullscreenOptimized(g_mainWindow);
    //        else
    //            SetOverlayAlwaysOnTop(g_mainWindow, false);

    //        g_overlayEnabled = !g_overlayEnabled;
    //    }

    //    if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
    //    {
    //        CleanupRenderTarget();
    //        g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
    //        g_ResizeWidth = g_ResizeHeight = 0;
    //        CreateRenderTarget();
    //    }

    //    ImGui_ImplDX11_NewFrame();
    //    ImGui_ImplWin32_NewFrame();
    //    ImGui::NewFrame();
    //    ImGui::Text("Hello, ImGui!");

    //    RenderOverlay();

    //    ImGui::Render();
    //    const float clear_color_with_alpha[4] = { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
    //    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

    //    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    //    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    //    g_pSwapChain->Present(1, 0); // Present with vsync
    //}
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }

        if (done)
            break;
        Sleep(10);

        HWND foregroundWindow = GetForegroundWindow();
        bool isFullscreen = IsFullscreenWindow(foregroundWindow);

        if ((isFullscreen && !g_overlayEnabled) || (!isFullscreen && g_overlayEnabled))
        {
            if (isFullscreen)
                SetOverlayAlwaysOnTopFullscreenOptimized(g_mainWindow);
            else
                SetOverlayAlwaysOnTop(g_mainWindow, false);

            g_overlayEnabled = !g_overlayEnabled;
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render ImGui overlay
        RenderOverlay();

        ImGui::EndFrame();  // Ensure ImGui frame is ended

        // Rendering
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        const float clear_color_with_alpha[4] = { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }



    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_mainWindow);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

void RenderOverlay()
{
    ImGui::Text("Rendering Overlay");

    if (g_overlayEnabled)
    {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // Use the updated ImGui context for getting the correct screen dimensions
        ImGuiIO& io = ImGui::GetIO();
        float centerX = io.DisplaySize.x / 2.0f;
        float centerY = io.DisplaySize.y / 2.0f;

        float crosshairSize = 10.0f;
        float lineWidth = 1.5f;
        float gapSize = 4.0f;
        float dotSize = 2.0f;

        drawList->AddLine({ centerX - crosshairSize - gapSize, centerY }, { centerX - gapSize, centerY }, ImColor(255, 255, 255), lineWidth);
        drawList->AddLine({ centerX + gapSize, centerY }, { centerX + crosshairSize + gapSize, centerY }, ImColor(255, 255, 255), lineWidth);

        drawList->AddLine({ centerX, centerY - crosshairSize - gapSize }, { centerX, centerY - gapSize }, ImColor(255, 255, 255), lineWidth);
        drawList->AddLine({ centerX, centerY + gapSize }, { centerX, centerY + crosshairSize + gapSize }, ImColor(255, 255, 255), lineWidth);

        drawList->AddCircleFilled({ centerX, centerY }, dotSize, ImColor(255, 255, 255));
    }
}


void SetOverlayTransparent(HWND hWnd)
{
    // Set the layered window style
    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    // Set the window to be fully opaque with no color key
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
}


void SetOverlayAlwaysOnTop(HWND hWnd, bool alwaysOnTop)
{
    SetWindowPos(hWnd, alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SetOverlayAlwaysOnTopFullscreenOptimized(HWND hWnd)
{
    SetOverlayAlwaysOnTop(hWnd, true);
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
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
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED)
    {
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }

    if (res != S_OK)
    {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = static_cast<UINT>(LOWORD(lParam));
        g_ResizeHeight = static_cast<UINT>(HIWORD(lParam));
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_ACTIVATEAPP:
        // Toggle overlay visibility based on whether your application is active
        if (wParam)
        {
            // Application is being activated
            // Show the overlay if needed
            if (IsFullscreenWindow(GetForegroundWindow()))
            {
                SetOverlayAlwaysOnTopFullscreenOptimized(hWnd);
            }
        }
        else
        {
            // Application is being deactivated
            // Hide the overlay if needed
            SetOverlayAlwaysOnTop(hWnd, false);
        }
        break;
    case WM_PAINT:
        // Handle paint if needed
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
