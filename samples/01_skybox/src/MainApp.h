#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <cstdlib>
#include <string>

#include <The-Forge/Common_3/Application/Interfaces/IApp.h>
#include <The-Forge/Common_3/Utilities/Interfaces/IFileSystem.h>
#include <The-Forge/Common_3/Application/Interfaces/IFont.h>
#include <The-Forge/Common_3/Graphics/Interfaces/IGraphics.h>
#include <The-Forge/Common_3/Application/Interfaces/IProfiler.h>
#include <The-Forge/Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h>
#include <The-Forge/Common_3/Application/Interfaces/IScreenshot.h>
#include <The-Forge/Common_3/Utilities/RingBuffer.h>

constexpr uint32_t gDataBufferCount = 2;

#include <input.h>
#include <quad.h>
#include <skybox.h>

class MainApp : public IApp
{
private:
    Renderer* pRenderer = nullptr;
    SwapChain* pSwapChain = nullptr;
    Queue* pGraphicsQueue = nullptr;
    GpuCmdRing gGraphicsCmdRing = {};
    Semaphore* pImageAcquiredSemaphore = nullptr;
    ProfileToken gGpuProfileToken = PROFILE_INVALID_TOKEN;


    mat4 viewMat;
    CameraMatrix projMat;

    RenderTarget* pRTDepth = nullptr;
    TinyImageFormat depthBufferFormat = TinyImageFormat_D32_SFLOAT;

    Skybox skybox;
    Quad quad;

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
