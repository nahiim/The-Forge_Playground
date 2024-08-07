#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <cstdlib>
#include <string>
#include <random>

#include <The-Forge/Common_3/Application/Interfaces/IApp.h>
#include <The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h>
#include <The-Forge/Common_3/Application/Interfaces/IFont.h>
#include <The-Forge/Common_3/Graphics/Interfaces/IGraphics.h>
#include <The-Forge/Common_3/Application/Interfaces/IProfiler.h>
#include <The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h>
#include <The-Forge/Common_3/Application/Interfaces/IScreenshot.h>
#include <The-Forge/Common_3/Utilities/RingBuffer.h>

constexpr uint32_t gDataBufferCount = 2;
constexpr uint32_t PARTICLE_COUNT = 8192;

#include <input.h>

struct Particle
{
    vec2 position;
    vec2 velocity;
    vec4 color;
};

struct UBO
{
    float delta_time;
}ubo;

class MainApp : public IApp
{
private:
    Renderer* pRenderer = nullptr;
    SwapChain* pSwapChain = nullptr;
    Queue* pGraphicsQueue = nullptr;
    GpuCmdRing gGraphicsCmdRing = {};
    Semaphore* pImageAcquiredSemaphore = nullptr;
    Semaphore* pRenderFinishedSemaphore = nullptr;
    Semaphore* pComputeFinishedSemaphore = nullptr;
    ProfileToken gGpuProfileToken = PROFILE_INVALID_TOKEN;

    Buffer* pSSBuffer = nullptr;
    Buffer* pUniformBuffer = nullptr;

    mat4 viewMat;
    CameraMatrix projMat;

    RenderTarget* pRTDepth = nullptr;
    RenderTarget* pRTColor= nullptr;

    TinyImageFormat depthBufferFormat = TinyImageFormat_D32_SFLOAT;

    Shader* pComputeShader = nullptr;
    Shader* pRasterShader = nullptr;
    RootSignature* pComputeRootSignature = nullptr;
    RootSignature* pRasterRootSignature = nullptr;
    DescriptorSet* pDescriptorSet = nullptr;
    Pipeline* pComputePipeline = nullptr;
    Pipeline* pGraphicsPipeline = nullptr;

    std::vector<Particle> particles;
    const uint32_t particle_stride = sizeof(Particle);

    uint32_t gFrameIndex = 0;

public:
    bool Init() override;
    void Exit() override;
    bool Load(ReloadDesc* pReloadDesc) override;
    void Unload(ReloadDesc* pReloadDesc) override;
    void Draw() override;
    void Update(float deltaTime) override;
    const char* GetName() override;
};

#endif // MAIN_H
