#include "d3d11va.h"

#include <SDL_syswm.h>
#include <VersionHelpers.h>

#define SAFE_COM_RELEASE(x) if (x) { (x)->Release(); }

D3D11VARenderer::D3D11VARenderer()
    : m_Device(nullptr),
      m_SwapChain(nullptr),
      m_DeviceContext(nullptr),
      m_HwContext(nullptr)
{

}

D3D11VARenderer::~D3D11VARenderer()
{
    SAFE_COM_RELEASE(m_SwapChain);

    if (m_HwContext != nullptr) {
        // This will release m_Device too
        av_buffer_unref(&m_HwContext);
    }
    else {
        SAFE_COM_RELEASE(m_Device);
    }

    SAFE_COM_RELEASE(m_DeviceContext);
}

bool D3D11VARenderer::initialize(PDECODER_PARAMETERS params)
{
    int adapterIndex, outputIndex;
    HRESULT result;

    if (!SDL_DXGIGetOutputInfo(SDL_GetWindowDisplayIndex(params->window),
                               &adapterIndex, &outputIndex)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_DXGIGetOutputInfo() failed: %s",
                     SDL_GetError());
        return false;
    }

    IDXGIFactory1* factory;
    result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    if (FAILED(result)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "CreateDXGIFactory1() failed: %x",
                     result);
        return false;
    }

    IDXGIAdapter1* adapter;
    result = factory->EnumAdapters1(adapterIndex, &adapter);
    if (FAILED(result)) {
        factory->Release();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "EnumAdapters1() failed: %x",
                     result);
        return false;
    }

    IDXGIOutput* output;
    result = adapter->EnumOutputs(outputIndex, &output);
    factory->Release();
    if (FAILED(result)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "EnumOutputs() failed: %x",
                     result);
        return false;
    }

    // TODO: Check VID and PID for codec support

    SDL_SysWMinfo info;

    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(params->window, &info);

    Uint32 windowFlags = SDL_GetWindowFlags(params->window);

    DXGI_SWAP_CHAIN_DESC swapChainDesc;

    SDL_zero(swapChainDesc);

    // TODO: We can use DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO here to use YUV overlay planes
    // but not all GPUs support it.

    swapChainDesc.OutputWindow = info.info.win.window;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    swapChainDesc.Windowed = true;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SampleDesc.Count = 1;

    if (IsWindows10OrGreater()) {
        // TODO: Is this supported for *all* Win10 systems?
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    }
    else {
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    }

    if (windowFlags & SDL_WINDOW_FULLSCREEN) {
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO;
    }

    if (!params->enableVsync) {
        // TODO: Check for support first
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    if (params->videoFormat == VIDEO_FORMAT_H265_MAIN10) {
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    }
    else {
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    IDXGISwapChain* swapChain;
    result = D3D11CreateDeviceAndSwapChain(adapter,
                                           D3D_DRIVER_TYPE_UNKNOWN,
                                           nullptr,
                                           D3D11_CREATE_DEVICE_VIDEO_SUPPORT
                                       #ifdef QT_DEBUG
                                               | D3D11_CREATE_DEVICE_DEBUG
                                       #endif
                                           ,
                                           nullptr,
                                           0,
                                           D3D11_SDK_VERSION,
                                           &swapChainDesc,
                                           &swapChain,
                                           &m_Device,
                                           nullptr,
                                           &m_DeviceContext);

    adapter->Release();

    if (FAILED(result)) {
        output->Release();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "D3D11CreateDeviceAndSwapChain() failed: %x",
                     result);
        return false;
    }

    result = swapChain->QueryInterface(__uuidof(IDXGISwapChain2), (void**)&m_SwapChain);
    swapChain->Release();

    if (FAILED(result)) {
        output->Release();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "QueryInterface(IDXGISwapChain2) failed: %x",
                     result);
        return false;
    }

    result = m_SwapChain->SetMaximumFrameLatency(1);
    if (FAILED(result)) {
        output->Release();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SetMaximumFrameLatency() failed: %x",
                     result);
        return false;
    }

    m_HwContext = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
    if (!m_HwContext) {
        output->Release();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to allocate D3D11VA context");
        return false;
    }

    AVHWDeviceContext* deviceContext = (AVHWDeviceContext*)m_HwContext->data;
    AVD3D11VADeviceContext* d3d11vaDeviceContext = (AVD3D11VADeviceContext*)deviceContext->hwctx;

    // AVHWDeviceContext takes ownership of this pointer
    d3d11vaDeviceContext->device = m_Device;

    int err = av_hwdevice_ctx_init(m_HwContext);
    if (err < 0) {
        output->Release();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to initialize D3D11VA context: %d",
                     err);
        return false;
    }

    // MSDN recommends always creating the swapchain as windowed, then switching
    // to full-screen with SetFullscreenState() after creation.
    if ((windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN) {
        m_SwapChain->SetFullscreenState(true, output);

        // TODO
        //m_SwapChain->ResizeBuffers()
    }

    output->Release();

    // Wait for the swap chain to be ready
    WaitForSingleObjectEx(m_SwapChain->GetFrameLatencyWaitableObject(), 100, FALSE);

    return true;
}

bool D3D11VARenderer::prepareDecoderContext(AVCodecContext* context)
{
    context->hw_device_ctx = av_buffer_ref(m_HwContext);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Using D3D11VA accelerated renderer");

    return true;
}

void D3D11VARenderer::renderFrame(AVFrame* frame)
{
    // TODO: DXGI_PRESENT_ALLOW_TEARING when v-sync off
    m_SwapChain->Present(0, DXGI_PRESENT_RESTART);

    // MSDN advises us to wait *before* doing any rendering operations,
    // however that assumes the a typical game which will latch inputs,
    // run the engine, draw, etc. after WaitForSingleObjectEx(). In our case,
    // we actually want wait *after* our rendering operations, because our AVFrame
    // is already set in stone by the time we enter this function. Waiting after
    // presenting allows a more recentframe to be received before renderFrame()
    // is called again.
    WaitForSingleObjectEx(m_SwapChain->GetFrameLatencyWaitableObject(), 100, FALSE);
}

void D3D11VARenderer::notifyOverlayUpdated(Overlay::OverlayType)
{
    // TODO
}

int D3D11VARenderer::getDecoderColorspace()
{
    // TODO
    return COLORSPACE_REC_601;
}
