// ModalTest.cpp - Test DesktopPopupSiteBridge modal behavior
// Compile with C++/WinRT support

#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Content.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Interop.h>

// Windows App SDK Bootstrap API
#include <MddBootstrap.h>

#pragma comment(lib, "Microsoft.WindowsAppRuntime.Bootstrap.lib")

using namespace winrt;
using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Windowing;
using namespace winrt::Microsoft::UI::Content;
using namespace winrt::Microsoft::UI::Composition;
using namespace winrt::Microsoft::UI::Dispatching;

// Forward declaration
void CloseModal();

// Global variables yes
HWND g_parentHwnd = nullptr;
DesktopPopupSiteBridge g_popup = nullptr;
AppWindow g_modalAppWindow = nullptr;
ContentIsland g_modalIsland = nullptr;
ContentIsland g_parentIsland = nullptr;
DesktopChildSiteBridge g_parentSiteBridge = nullptr;
bool g_modalVisible = false;

void CreateModalWindow()
{
    if (g_popup)
        return; // Already created

    // Create ContentIsland for the modal with visible content
    auto compositor = Compositor();
    auto rootVisual = compositor.CreateSpriteVisual();
    rootVisual.Size({ 400, 300 });
    
    // Add a light gray background to make the modal visible
    rootVisual.Brush(compositor.CreateColorBrush(winrt::Windows::UI::Color{ 255, 240, 240, 240 }));
    
    // Add a blue rectangle as content
    auto blueRect = compositor.CreateSpriteVisual();
    blueRect.Size({ 300, 200 });
    blueRect.Offset({ 50, 50, 0 });
    blueRect.Brush(compositor.CreateColorBrush(winrt::Windows::UI::Color{ 255, 0, 120, 215 }));
    rootVisual.Children().InsertAtTop(blueRect);
    
    g_modalIsland = ContentIsland::Create(rootVisual);

    if (!g_parentIsland)
    {
        OutputDebugStringW(L"ERROR: Parent island not created!\n");
        return;
    }
    
    // Create DesktopPopupSiteBridge using the real parent island
    g_popup = DesktopPopupSiteBridge::Create(g_parentIsland);
    g_popup.Connect(g_modalIsland);

    // Get AppWindow from the popup
    g_modalAppWindow = AppWindow::GetFromWindowId(g_popup.WindowId());
    
    if (g_modalAppWindow)
    {
        auto presenter = OverlappedPresenter::Create();
        
        // Configure as modal - THIS IS THE KEY PART WE'RE TESTING
        presenter.IsModal(true);
        presenter.SetBorderAndTitleBar(true, true);
        
        g_modalAppWindow.SetPresenter(presenter);
        g_modalAppWindow.Title(L"Modal Test Window - Hello from Modal!");
        
        // Handle window closing event - MATCH RNW BEHAVIOR
        g_modalAppWindow.Closing([](auto sender, auto args) {
            OutputDebugStringW(L"Modal X button clicked - Closing event fired\n");
            args.Cancel(true); // Prevent default close - JUST LIKE RNW
            OutputDebugStringW(L"Calling CloseModal() to DESTROY modal (close island and popup)\n");
            
            // Call CloseModal to destroy everything (simulates RNW destructor)
            CloseModal();
        });
        
        g_modalAppWindow.Changed([](auto sender, auto args) {
            if (args.DidVisibilityChange() && !sender.IsVisible())
            {
                OutputDebugStringW(L"Modal window became invisible\n");
            }
        });
        
        // Resize to a reasonable size
        g_modalAppWindow.ResizeClient({400, 300});
        
        // Center the modal on the parent window
        if (g_parentHwnd)
        {
            RECT parentRect;
            GetWindowRect(g_parentHwnd, &parentRect);
            int parentWidth = parentRect.right - parentRect.left;
            int parentHeight = parentRect.bottom - parentRect.top;
            
            int modalX = parentRect.left + (parentWidth - 400) / 2;
            int modalY = parentRect.top + (parentHeight - 300) / 2;
            
            g_modalAppWindow.Move({modalX, modalY});
        }
        
        OutputDebugStringW(L"Modal window created and positioned\n");
    }
}

void ShowModal()
{
    CreateModalWindow();
    
    if (g_popup && !g_modalVisible)
    {
        g_popup.Show();
        
        // Also explicitly show the AppWindow
        if (g_modalAppWindow)
        {
            g_modalAppWindow.Show();
        }
        
        g_modalVisible = true;
        OutputDebugStringW(L"Modal shown\n");
        
        // Debug: Check if window is actually visible
        if (g_modalAppWindow)
        {
            WCHAR debugMsg[256];
            swprintf_s(debugMsg, L"AppWindow IsVisible: %d\n", g_modalAppWindow.IsVisible());
            OutputDebugStringW(debugMsg);
        }
    }
}

void CloseModal()
{
    if (g_popup && g_modalVisible)
    {
        OutputDebugStringW(L"=== TEST: Simulating RNW destructor - DESTROYING modal ===\n");
        
        // Hide the popup
        if (g_popup)
        {
            OutputDebugStringW(L"Step 0: Hiding popup...\n");
            g_popup.Hide();
            OutputDebugStringW(L"Popup hidden\n");
        }
        
        // Re-enable parent window
        if (g_parentHwnd)
        {
            EnableWindow(g_parentHwnd, TRUE);
            SetForegroundWindow(g_parentHwnd);
            OutputDebugStringW(L"Parent window re-enabled\n");
        }
        
		// Close popup desktop child site bridge
        if (g_popup)
        {
            OutputDebugStringW(L"Step 1: Closing popup...\n");
            g_popup.Close();
            OutputDebugStringW(L"Popup closed\n");
            g_popup = nullptr;
        }
        
        //  Close modal island
        if (g_modalIsland)
        {
            OutputDebugStringW(L"Step 2: Closing modal island...\n");
            g_modalIsland.Close();
            OutputDebugStringW(L"Modal island closed\n");
            g_modalIsland = nullptr;
        }


        // Step 3: Close modal AppWindow
        if (g_modalAppWindow)
        {
            OutputDebugStringW(L"Step 3: Nulling AppWindow...\n");
            // AppWindow is already closed when popup closes
            g_modalAppWindow = nullptr;
        }
        
        g_modalVisible = false;
        OutputDebugStringW(L"=== Modal fully DESTROYED ===\n");
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            // Create buttons
            CreateWindowW(L"BUTTON", L"Show Modal",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 10, 120, 30,
                hWnd, (HMENU)1, nullptr, nullptr);
            
            CreateWindowW(L"BUTTON", L"Close Modal",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                140, 10, 120, 30,
                hWnd, (HMENU)2, nullptr, nullptr);
        }
        break;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) // Show Modal button
        {
            ShowModal();
        }
        else if (LOWORD(wParam) == 2) // Close Modal button
        {
            CloseModal();
        }
        break;
        
    case WM_LBUTTONDOWN:
        {
            // TEST: Click on parent window after closing modal
            // If this causes a crash, we have the same issue as React Native
            WCHAR msg[256];
            swprintf_s(msg, L"Parent window clicked at (%d, %d)\n", 
                LOWORD(lParam), HIWORD(lParam));
            OutputDebugStringW(msg);
            MessageBoxW(hWnd, L"Parent window is responsive!", L"Test", MB_OK);
        }
        break;
        
    case WM_DESTROY:
        if (g_popup)
        {
            g_popup.Close();
            g_popup = nullptr;
        }
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // Initialize Windows App SDK
    const UINT32 majorMinorVersion{ 0x00010007 }; // 1.7
    PCWSTR versionTag{ L"" };
    const PACKAGE_VERSION minVersion{ 250401001 }; // 1.7.250401001
    
    HRESULT hr = MddBootstrapInitialize2(majorMinorVersion, versionTag, minVersion, MddBootstrapInitializeOptions_None);
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to initialize Windows App SDK", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // Initialize WinRT
    init_apartment();
    
    // Create DispatcherQueue on the current thread (required for Compositor)
    auto controller = DispatcherQueueController::CreateOnCurrentThread();
    
    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModalTestWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    RegisterClassW(&wc);
    
    // Create main window
    g_parentHwnd = CreateWindowExW(
        0, L"ModalTestWindow", L"Modal IsModal Test - Parent Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        nullptr, nullptr, hInstance, nullptr);
    
    if (!g_parentHwnd)
        return 1;
    
    ShowWindow(g_parentHwnd, nCmdShow);
    
    // Create parent ContentIsland connected to the main window
    // This is required for DesktopPopupSiteBridge::Create()
    auto compositor = Compositor();
    auto parentVisual = compositor.CreateSpriteVisual();
    parentVisual.Size({ 800, 600 });
    g_parentIsland = ContentIsland::Create(parentVisual);
    
    // Connect the island to the parent HWND using DesktopChildSiteBridge
    auto parentWindowId = winrt::Microsoft::UI::GetWindowIdFromWindow(g_parentHwnd);
    g_parentSiteBridge = DesktopChildSiteBridge::Create(compositor, parentWindowId);
    g_parentSiteBridge.Connect(g_parentIsland);
    
    OutputDebugStringW(L"Parent ContentIsland created and connected to HWND\n");
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup Windows App SDK
    MddBootstrapShutdown();
    
    return 0;
}
