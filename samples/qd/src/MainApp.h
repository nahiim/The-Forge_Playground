#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <vector>

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
#include <skybox.h>

struct QuadUBO
{
    vec3 color;
    mat4 model;
    CameraMatrix projectView;
}quadUBO;

struct Vertex
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
};

class MainApp : public IApp
{
private:
    Renderer* pRenderer = nullptr;
    SwapChain* pSwapChain = nullptr;
    Queue* pGraphicsQueue = nullptr;
    GpuCmdRing gGraphicsCmdRing = {};
    Semaphore* pImageAcquiredSemaphore = nullptr;
    Semaphore* pRenderFinishedSemaphore = nullptr;
    ProfileToken gGpuProfileToken = PROFILE_INVALID_TOKEN;

    Buffer* pQuadVB= nullptr;
    Buffer* pQuadUB= nullptr;
    Shader* pQuadShader = nullptr;
    RootSignature* pQuadRS= nullptr;
    DescriptorSet* pQuadDS= nullptr;
    Pipeline* pQuadPipeline = nullptr;
    Buffer* pQuadIB = nullptr;

    Sampler* pSampler = NULL;
    Texture* pTexture;
    DescriptorSet* pTextureDS = { NULL };
    const char* diffusePath = "circlepad.tex";

    vec3 upVector = vec3(0.0f, 1.0f, 0.0f);
    uint32_t n = 50;
    unsigned int quadVertexCount = n * n;

    vec3 *positions = new vec3[quadVertexCount];
    vec2 *texCoords = new vec2[quadVertexCount];
    vec3 *normals   = new vec3[quadVertexCount];

    std::vector<vec3> tangents;

    uint32_t indexCount = 6 * (n - 1) * (n - 1);   // Number of indices
    uint32_t *indices = new unsigned int[indexCount];

    vec3 *positionPtr = positions;
    vec2 *texCoordPtr = texCoords;
    vec3 *normalPtr = normals;
    uint32_t *indexPtr = indices;

    std::vector<Vertex> vertices;

    mat4 viewMat;
    CameraMatrix projMat;

    RenderTarget* pRTDepth = nullptr;
    TinyImageFormat depthBufferFormat = TinyImageFormat_D32_SFLOAT;

    Skybox skybox;

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
