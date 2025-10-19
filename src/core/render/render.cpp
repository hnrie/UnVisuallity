#include "render.h"

#include "globals.h"
#include "../../rbx/taskscheduler/taskscheduler.h"
#include "xorstr/xorstr.h"
#include "mutex"
#include "src/rbx/engine/game.h"
#include "src/misc/drawing_structures.h"
#include "src/misc/inter.h"
#include "src/misc/jetbrains_mono.h"
#include "user_interface/key_interface.hpp"

HWND renderer::windowhandle = nullptr;
IDXGISwapChain* renderer::swapchain = nullptr;
ID3D11Device* renderer::device = nullptr;
ID3D11DeviceContext* renderer::devicecontext = nullptr;
ID3D11RenderTargetView* renderer::rendertargetview = nullptr;
ID3D11Texture2D* renderer::backbuffer = nullptr;

renderer::presentfn renderer::originalpresent = nullptr;
renderer::resizebuffersfn renderer::originalresizebuffers = nullptr;
WNDPROC renderer::originalwindowproc = nullptr;

float renderer::dpi_scale = 0;
bool renderer::test = false;
ImFont* renderer::sigma_font = nullptr;

std::recursive_mutex _mutex { };

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void renderer::initialize() {
    uintptr_t renderjob = g_taskscheduler->get_job_by_name(OBF("RenderJob"));

    while (!renderjob)
    {
        renderjob = g_taskscheduler->get_job_by_name(OBF("RenderJob"));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    uintptr_t viewbase = *(uintptr_t*)(renderjob + 0x218);
    uintptr_t deviceaddr = *(uintptr_t*)(viewbase + 0x8);

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
    /*else if ( msg == WM_SIZE ) {
        std::uint32_t width = LOWORD( lparam ), height = HIWORD( lparam );
        if ( !is_render_hooked && ( window_width != width || window_height != height ) ) {
            window_width = width, window_height = height;
            window_size_changes.push( true );
        }
    }*/

    if ((test /*|| !globals::authenticated*/ ) && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    switch (msg) {
    case 522:
    case 513:
    case 533:
    case 514:
    case 134:
    case 516:
    case 517:
    case 258:
    case 257:
    case 132:
    case 127:
    case 255:
    case 523:
    case 524:
    case 793:
        if ( test /*|| !globals::authenticated*/ )
            return true;
        break;
    }

    return CallWindowProc(originalwindowproc, hwnd, msg, wparam, lparam);
}

HRESULT WINAPI renderer::resizebuffers(IDXGISwapChain* InSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    std::unique_lock lock( _mutex );

    if (niggachain::Renderer) // IF CRASH OR SMTH THEN IT MIGHT BE RELATED TO TS
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

    //IO.Fonts->AddFontDefault();

    static const ImWchar ranges[] = {
            0x0020, 0x00FF,0x2000, 0x206F,0x3000, 0x30FF,0x31F0, 0x31FF, 0xFF00,
            0xFFEF,0x4e00, 0x9FAF,0x0400, 0x052F,0x2DE0, 0x2DFF,0xA640, 0xA69F, 0
    };

    //g_user_interface->text_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    //g_user_interface->text_editor.SetPalette(TextEditor::GetDarkPalette());
    //g_user_interface->text_editor.SetShowWhitespaces(false);

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

        std::call_once(init_flag, []() { if (niggachain::Renderer) niggachain::Renderer->initializeimgui(); });
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();



        ImGui::NewFrame();
        {

            for (const auto& drawing_object: drawing_cache)
                drawing_object->draw_obj();
        }
        ImGui::Render();

        ImDrawData* data = ImGui::GetDrawData( );
      /*


          for ( std::int32_t i = 0; i < data->CmdListsCount; i++ ) {
              ImDrawList* cmd_list = data->CmdLists[ i ];

              for ( std::int32_t cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++ ) {
                  ImDrawCmd* cmd = &cmd_list->CmdBuffer[ cmd_i ];
                  cmd->ClipRect = ImVec4( cmd->ClipRect.x * dpi_scale, cmd->ClipRect.y * dpi_scale, cmd->ClipRect.z * dpi_scale, cmd->ClipRect.w * dpi_scale );
              }

              for ( auto& cmd : data->CmdLists[ i ]->VtxBuffer ) {
                  cmd.pos.x *= dpi_scale;
                  cmd.pos.y *= dpi_scale;
              }
          }
       */

        devicecontext->OMSetRenderTargets(1, &rendertargetview, nullptr);

        ImGui_ImplDX11_RenderDrawData(data);
    }
    catch ( ... )
    {
        // do stuff
    }


    return originalpresent( swap_chain, sync_interval, flags );
    /*
     * static std::once_flag InitFlag;
    std::call_once(InitFlag, []() { if (niggachain::Renderer) initializeimgui(); });

    if (!niggachain::Renderer) return S_OK;

    if (!rendertargetview) {
        ID3D11Texture2D* BackBuffer = nullptr;

        if (SUCCEEDED(InSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer)))) {
            device->CreateRenderTargetView(BackBuffer, nullptr, &rendertargetview);
            BackBuffer->Release();
        }
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    if (test) {

        g_user_interface->render();

    }

    ImGui::Render();
    devicecontext->OMSetRenderTargets(1, &rendertargetview, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return originalpresent(InSwapChain, SyncInterval, Flags);
     */
}