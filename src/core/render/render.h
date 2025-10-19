#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>
#include <shared_mutex>
#include <unordered_set>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

class renderer {
private:
    static HWND windowhandle;
    static IDXGISwapChain* swapchain;
    static ID3D11Device* device;
    static ID3D11DeviceContext* devicecontext;
    static ID3D11RenderTargetView* rendertargetview;
    static ID3D11Texture2D* backbuffer;

    static bool test;
    static float dpi_scale;

    using presentfn = int(WINAPI*)(IDXGISwapChain *const swap_chain, const std::uint32_t sync_interval,
                                   const std::uint32_t flags);
    using resizebuffersfn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    static presentfn originalpresent;
    static resizebuffersfn originalresizebuffers;
    static WNDPROC originalwindowproc;

    static void render();
    bool initializeimgui();
    static void cleanupimgui();

    static HRESULT WINAPI present(IDXGISwapChain* InSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT WINAPI resizebuffers(IDXGISwapChain* InSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    static LRESULT CALLBACK windowprochandler(HWND Hwnd, UINT Message, WPARAM WParam, LPARAM LParam);

    template<typename T>
    static void saferelease(T*& ptr) {
        if (ptr) {
            ptr->Release();
            ptr = nullptr;
        }
    }

public:
    static ImFont* sigma_font;

	static void initialize();
    [[nodiscard]] ID3D11Device* get_device() const { return this->device; };
};

namespace niggachain {
    inline auto Renderer = std::make_unique<renderer>();
}
