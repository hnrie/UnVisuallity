#include "render.h"

#include "globals.h"
#include "../../rbx/taskscheduler/taskscheduler.h"
#include "xorstr/xorstr.h"
#include <mutex>
#include "src/rbx/engine/game.h"
#include "src/misc/drawing_structures.h"
#include "src/misc/inter.h"
#include "src/misc/jetbrains_mono.h"
#include "user_interface/key_interface.hpp"

#include <thread>

HWND renderer::windowhandle = nullptr;
IDXGISwapChain* renderer::swapchain = nullptr;
ID3D11Device* renderer::device = nullptr;
ID3D11DeviceContext* renderer::devicecontext = nullptr;
ID3D11RenderTargetView* renderer::rendertargetview = nullptr;
ID3D11Texture2D* renderer::backbuffer = nullptr;

renderer::presentfn renderer::originalpresent = nullptr;
renderer::resizebuffersfn renderer::originalresizebuffers = nullptr;
WNDPROC renderer::originalwindowproc = nullptr;

float renderer::dpi_scale = 1.0f;
bool renderer::test = false;
ImFont* renderer::sigma_font = nullptr;

std::recursive_mutex _mutex { };

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void renderer::initialize() {
    uintptr_t renderjob = scheduler_global::instance->get_job_by_name(OBF("RenderJob"));

    while (!renderjob)
    {
        renderjob = scheduler_global::instance->get_job_by_name(OBF("RenderJob"));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!renderjob) return;

    uintptr_t viewbase = *(uintptr_t*)(renderjob + 0x218);
    if (!viewbase) return;

    uintptr_t deviceaddr = *(uintptr_t*)(viewbase + 0x8);
    if (!deviceaddr) return;

    swapchain = *(IDXGISwapChain**)(deviceaddr + 0xC8);
    if (!swapchain)
        return;

    DXGI_SWAP_CHAIN_DESC swapchaindesc;
    if (FAILED(swapchain->GetDesc(&swapchaindesc)))
        return;


    windowhandle = swapchaindesc.OutputWindow;

    if (FAILED(swapchain->GetDevice(__uuidof(ID3D11Device), (void**)(&device))))
        return;

    device->GetImmediateContext(&devicecontext);

    void** original_vtable = *(void***)(swapchain);
    constexpr size_t vtable_sz = 18;

    auto shadow_vtable = std::make_unique<void* []>(vtable_sz);
    memcpy(shadow_vtable.get(), original_vtable, sizeof(void*) * vtable_sz);

    originalpresent = (presentfn)(original_vtable[8]);
    shadow_vtable[8] = (void*)(&present);

    originalresizebuffers = (resizebuffersfn)(original_vtable[13]);
    shadow_vtable[13] = (void*)(&resizebuffers);

    *(void***)(swapchain) = shadow_vtable.release();
    originalwindowproc = (WNDPROC)(SetWindowLongPtrW(windowhandle, GWLP_WNDPROC, (LONG_PTR)(windowprochandler)));
}

LRESULT CALLBACK renderer::windowprochandler(HWND hwnd, std::uint32_t msg, std::uint64_t wparam, std::int64_t lparam) {
    if (msg == WM_KEYDOWN) {
        if (wparam == VK_INSERT || wparam == VK_DELETE || wparam == VK_END) {
            test = !test;
        }
    }
    else if (msg == WM_DPICHANGED) {
        dpi_scale = LOWORD(wparam) / 96.0f;
    }

    if (test && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    if (test) {
         switch (msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                return true;
        }
    }

    return CallWindowProc(originalwindowproc, hwnd, msg, wparam, lparam);
}

HRESULT WINAPI renderer::resizebuffers(IDXGISwapChain* InSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    std::unique_lock lock( _mutex );

    if (render_global::instance) 
        saferelease(rendertargetview);

    return originalresizebuffers(InSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}


bool renderer::initializeimgui() {
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    IO.IniFilename = nullptr;

    ImGuiStyle& style = ImGui::GetStyle();

    style.IndentSpacing                     = 25;
    style.ScrollbarSize                     = 15;
    style.GrabMinSize                       = 10;
    style.WindowBorderSize                  = 1;
    style.ChildBorderSize                   = 1;
    style.PopupBorderSize                   = 1;
    style.FrameBorderSize                   = 1;
    style.TabBorderSize                     = 1;
    style.WindowRounding                    = 7;
    style.ChildRounding                     = 4;
    style.FrameRounding                     = 3;
    style.PopupRounding                     = 4;
    style.ScrollbarRounding                 = 9;
    style.GrabRounding                      = 3;
    style.LogSliderDeadzone                 = 4;
    style.TabRounding                       = 4;

    static const ImWchar ranges[] = {
            0x0020, 0x00FF,0x2000, 0x206F,0x3000, 0x30FF,0x31F0, 0x31FF, 0xFF00,
            0xFFEF,0x4e00, 0x9FAF,0x0400, 0x052F,0x2DE0, 0x2DFF,0xA640, 0xA69F, 0
    };

    ImFontConfig Config{ };
    Config.OversampleH = 3;
    Config.OversampleV = 3;

    if (!rendertargetview) {
        ID3D11Texture2D* BackBuffer = nullptr;

        if (SUCCEEDED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer)))) {
            device->CreateRenderTargetView(BackBuffer, nullptr, &rendertargetview);
            BackBuffer->Release();
        }
        else
            return false;
    }

    ImGui_ImplWin32_Init(windowhandle);
    ImGui_ImplDX11_Init(device, devicecontext);

    sigma_font = IO.Fonts->AddFontFromMemoryTTF(jetbrains_mono_data, sizeof(jetbrains_mono_data), 18, &Config, ranges);
    IO.FontDefault = sigma_font;

    return true;
}

static std::once_flag init_flag;
HRESULT WINAPI renderer::present(IDXGISwapChain *const swap_chain, const std::uint32_t sync_interval,
                                 const std::uint32_t flags) {
    std::unique_lock lock { _mutex, std::try_to_lock };
    if (!lock.owns_lock())
        return originalpresent( swap_chain, sync_interval, flags );

    try
    {
        if (!rendertargetview) {
            ID3D11Texture2D* BackBuffer = nullptr;

            if (SUCCEEDED(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer)))) {

                device->CreateRenderTargetView(BackBuffer, nullptr, &rendertargetview);
                BackBuffer->Release();
            }
        }

        std::call_once(init_flag, []() { if (render_global::instance) render_global::instance->initializeimgui(); });
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        {

            for (const auto& drawing_object: drawing_cache)
                drawing_object->draw_obj();
        }
        ImGui::Render();

        ImDrawData* data = ImGui::GetDrawData( );
        devicecontext->OMSetRenderTargets(1, &rendertargetview, nullptr);

        ImGui_ImplDX11_RenderDrawData(data);
    }
    catch ( ... )
    {
        // do stuff
    }


    return originalpresent( swap_chain, sync_interval, flags );
}
