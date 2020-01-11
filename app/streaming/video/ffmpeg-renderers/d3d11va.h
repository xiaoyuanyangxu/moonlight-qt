#pragma once

#include "renderer.h"
#include "pacer/pacer.h"

#include <d3d11_1.h>
#include <dxgi1_3.h>

extern "C" {
#include <libavutil/hwcontext_d3d11va.h>
}

class D3D11VARenderer : public IFFmpegRenderer
{
public:
    D3D11VARenderer();
    virtual ~D3D11VARenderer() override;
    virtual bool initialize(PDECODER_PARAMETERS params) override;
    virtual bool prepareDecoderContext(AVCodecContext* context) override;
    virtual void renderFrame(AVFrame* frame) override;
    virtual void notifyOverlayUpdated(Overlay::OverlayType) override;
    virtual int getDecoderColorspace() override;

private:
    ID3D11Device* m_Device;
    IDXGISwapChain2* m_SwapChain;
    ID3D11DeviceContext* m_DeviceContext;

    AVBufferRef* m_HwContext;
};

