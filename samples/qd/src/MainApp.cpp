
#include "MainApp.h"


void computeTangents(std::vector<vec3> &tangents, vec2* texCoords, uint32_t quadVertexCount)
{
    for (int i = 0; i < quadVertexCount; i += 3)
    {
        // Shortcuts for UVs
        vec2 uv0 = texCoords[i + 0];
        vec2 uv1 = texCoords[i + 1];
        vec2 uv2 = texCoords[i + 2];

        // UV delta
        vec2 deltaUV1 = uv1 - uv0;
        vec2 deltaUV2 = uv2 - uv0;

        vec3 tangent = vec3(deltaUV1.getX(), deltaUV1.getY(), 0.0f);


        // Set the same tangent for all three vertices of the triangle.
        tangents.push_back(tangent);
        tangents.push_back(tangent);
        tangents.push_back(tangent);
    }
}


void fillGrid(
    vec3* positionPtr, vec2* texCoordPtr, vec3* normalPtr,
    uint32_t* indexPtr, vec3 upVector, uint32_t n,
    std::vector<vec3> tangents, vec2* texCoords, uint32_t quadVertexCount)
{
    for (unsigned int x = 0; x < n; x++) for (unsigned int z = 0; z < n; z++)
    {
        *positionPtr++ = vec3((z / (float)n) - 0.5f, 0, (x / (float)n) - 0.5f);

        *texCoordPtr++ = vec2((z / (float)n) * n, (x / (float)n) * n);

        *normalPtr++ = upVector;
    }

    for (unsigned int x = 0; x < n - 1; x++) for (unsigned int z = 0; z < n - 1; z++)
    {
        *indexPtr++ = (x * n) + z;
        *indexPtr++ = ((x + 1) * n) + z;
        *indexPtr++ = ((x + 1) * n) + z + 1;

        *indexPtr++ = (x * n) + z;
        *indexPtr++ = ((x + 1) * n) + z + 1;
        *indexPtr++ = (x * n) + z + 1;
    }

    computeTangents(tangents, texCoords, quadVertexCount);
}

bool MainApp::Init()
{
    extern PlatformParameters gPlatformParameters;
    gPlatformParameters.mSelectedRendererApi = RendererApi::RENDERER_API_VULKAN;

    // FILE PATHS
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "CompiledShaders");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");
    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_DEBUG, "Debug");


    // window and renderer setup
    RendererDesc settings{};

    initRenderer(GetName(), &settings, &pRenderer);
    if (!pRenderer)
    {
        return false;
    }

    QueueDesc queueDesc = {};
    queueDesc.mType = QUEUE_TYPE_GRAPHICS;
    queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
    addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

    GpuCmdRingDesc cmdRingDesc = {};
    cmdRingDesc.pQueue = pGraphicsQueue;
    cmdRingDesc.mPoolCount = gDataBufferCount;
    cmdRingDesc.mCmdPerPoolCount = 1;
    cmdRingDesc.mAddSyncPrimitives = true;
    addGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

    addSemaphore(pRenderer, &pImageAcquiredSemaphore);
    addSemaphore(pRenderer, &pRenderFinishedSemaphore);

    initResourceLoaderInterface(pRenderer);
    skybox.init(pRenderer);
    
    fillGrid(positionPtr, texCoordPtr, normalPtr, indexPtr, upVector, n, tangents, texCoords, quadVertexCount);

    vertices.resize(quadVertexCount);
    for (size_t i = 0; i < quadVertexCount; ++i)
    {
        vertices[i].position = positions[i];
        vertices[i].texcoord = texCoords[i];
        vertices[i].normal = normals[i];
    }

    BufferLoadDesc quadVBDesc = {};
    quadVBDesc.ppBuffer = &pQuadVB;
    quadVBDesc.mDesc.mStructStride = sizeof(Vertex);
    quadVBDesc.mDesc.mElementCount = quadVertexCount;
    quadVBDesc.pData = vertices.data();
    quadVBDesc.mDesc.mSize = sizeof(Vertex) * quadVertexCount;
    quadVBDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    quadVBDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    addResource(&quadVBDesc, nullptr);

    BufferLoadDesc quadIBDesc = {};
    quadIBDesc.ppBuffer = &pQuadIB;
    quadIBDesc.pData = indices;
    quadIBDesc.mDesc.mSize = sizeof(uint32_t) * indexCount;
    quadIBDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    quadIBDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
    addResource(&quadIBDesc, nullptr);

    BufferLoadDesc quadUBDesc = {};
    quadUBDesc.ppBuffer = &pQuadUB;
    quadUBDesc.mDesc.mSize = sizeof(quadUBO);
    quadUBDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    quadUBDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    quadUBDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    addResource(&quadUBDesc, nullptr);

    TextureLoadDesc textureDesc = {};
    textureDesc.pFileName = diffusePath;
    textureDesc.ppTexture = &pTexture;
    // Textures representing color should be stored in SRGB or HDR format
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);

    // Dynamic sampler that is bound at runtime
    SamplerDesc samplerDesc = { FILTER_LINEAR,
                                FILTER_LINEAR,
                                MIPMAP_MODE_NEAREST,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE };
    addSampler(pRenderer, &samplerDesc, &pSampler);

    waitForAllResourceLoads();

    Input::Init(pRenderer, pWindow);

    return true;
}


bool MainApp::Load(ReloadDesc* pReloadDesc)
{
    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
    {
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.ppPresentQueues = &pGraphicsQueue;
        swapChainDesc.mPresentQueueCount = 1;
        swapChainDesc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
        swapChainDesc.mWidth = static_cast<uint32_t>(mSettings.mWidth);
        swapChainDesc.mHeight = static_cast<uint32_t>(mSettings.mHeight);
        swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
        swapChainDesc.mFlags = SWAP_CHAIN_CREATION_FLAG_ENABLE_FOVEATED_RENDERING_VR;
        swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
        swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
        addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

        RenderTargetDesc desc{};
        desc.mFlags = TEXTURE_CREATION_FLAG_ON_TILE | TEXTURE_CREATION_FLAG_VR_MULTIVIEW;
        desc.mWidth = pSwapChain->ppRenderTargets[0]->mWidth;
        desc.mHeight = pSwapChain->ppRenderTargets[0]->mHeight;
        desc.mDepth = 1;
        desc.mArraySize = 1;
        desc.mSampleCount = SAMPLE_COUNT_1;
        desc.mFormat = depthBufferFormat;
        desc.mStartState = RESOURCE_STATE_DEPTH_WRITE;
        desc.mClearValue = {};
        desc.mSampleQuality = 0;
        addRenderTarget(pRenderer, &desc, &pRTDepth);
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        skybox.loadShaders(pRenderer);

        ShaderLoadDesc quadShaderDesc{};
        quadShaderDesc.mStages[0].pFileName = "quad.vert";
        quadShaderDesc.mStages[1].pFileName = "quad.frag";
        addShader(pRenderer, &quadShaderDesc, &pQuadShader);

        RootSignatureDesc quadRSDesc{};
        quadRSDesc.ppShaders = &pQuadShader;
        quadRSDesc.mShaderCount = 1;
        addRootSignature(pRenderer, &quadRSDesc, &pQuadRS);

        DescriptorSetDesc quadDSDesc;
        quadDSDesc.mMaxSets = 1;
        quadDSDesc.mUpdateFrequency = DESCRIPTOR_UPDATE_FREQ_NONE;
        quadDSDesc.pRootSignature = pQuadRS;
        addDescriptorSet(pRenderer, &quadDSDesc, &pQuadDS);

        DescriptorSetDesc desc = { pQuadRS, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
        addDescriptorSet(pRenderer, &desc, &pTextureDS);


        RasterizerStateDesc rasterizerStateDesc = {};
        rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
        RasterizerStateDesc wireframeRasterizerStateDesc = {};
        wireframeRasterizerStateDesc.mCullMode = CULL_MODE_NONE;
        wireframeRasterizerStateDesc.mFillMode = FILL_MODE_WIREFRAME;

        DepthStateDesc depthStateDesc = {};
        depthStateDesc.mDepthTest = true;
        depthStateDesc.mDepthWrite = true;
        depthStateDesc.mDepthFunc = CMP_GEQUAL;

        skybox.attachPipeline(pRenderer, pSwapChain, rasterizerStateDesc, depthStateDesc);

        VertexLayout vertexLayout = {};
        vertexLayout.mBindingCount = 1;
        vertexLayout.mBindings[0].mStride = sizeof(Vertex);

        vertexLayout.mAttribCount = 3;
        vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[0].mBinding = 0;
        vertexLayout.mAttribs[0].mLocation = 0;
        vertexLayout.mAttribs[0].mOffset = 0;

        vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
        vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32_SFLOAT;
        vertexLayout.mAttribs[1].mBinding = 0;
        vertexLayout.mAttribs[1].mLocation = 1;
        vertexLayout.mAttribs[1].mOffset = offsetof(Vertex, texcoord);

        vertexLayout.mAttribs[2].mSemantic = SEMANTIC_NORMAL;
        vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[2].mBinding = 0;
        vertexLayout.mAttribs[2].mLocation = 2;
        vertexLayout.mAttribs[2].mOffset = offsetof(Vertex, normal);


        PipelineDesc quadPipelineDesc = {};
        quadPipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;
        quadPipelineDesc.mGraphicsDesc = {};
        quadPipelineDesc.mGraphicsDesc.pShaderProgram = pQuadShader;
        quadPipelineDesc.mGraphicsDesc.pRootSignature = pQuadRS;
        quadPipelineDesc.mGraphicsDesc.pVertexLayout = &vertexLayout;
        quadPipelineDesc.mGraphicsDesc.pDepthState = &depthStateDesc;
        quadPipelineDesc.mGraphicsDesc.pRasterizerState = &wireframeRasterizerStateDesc;
        quadPipelineDesc.mGraphicsDesc.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        quadPipelineDesc.mGraphicsDesc.mRenderTargetCount = 1;
        quadPipelineDesc.mGraphicsDesc.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        quadPipelineDesc.mGraphicsDesc.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        quadPipelineDesc.mGraphicsDesc.mDepthStencilFormat = TinyImageFormat_D32_SFLOAT;
        quadPipelineDesc.mGraphicsDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        quadPipelineDesc.mGraphicsDesc.mVRFoveatedRendering = true;
        addPipeline(pRenderer, &quadPipelineDesc, &pQuadPipeline);
    }

    skybox.prepareDescriptorSets(pRenderer);

    // Prepare Descriptore sets
    DescriptorData params[2] = {};
    params[0].pName = "quadUBO";
    params[0].ppBuffers = &pQuadUB;
    params[1].pName = "uSampler0";  // Ensure this name matches your shader
    params[1].ppSamplers = &pSampler;
    updateDescriptorSet(pRenderer, 0, pQuadDS, 2, params);

    waitForAllResourceLoads();

    return true;
}


void MainApp::Draw()
{
    if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
    {
        waitQueueIdle(pGraphicsQueue);
        ::toggleVSync(pRenderer, &pSwapChain);
    }
    uint32_t swapchainImageIndex;
    acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, nullptr, &swapchainImageIndex);

    RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
    GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);

    // Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
    {
        waitForFences(pRenderer, 1, &elem.pFence);
    }

    skybox.updateBuffer(gFrameIndex);

    // Reset cmd pool for this frame
    resetCmdPool(pRenderer, elem.pCmdPool);

    Cmd* cmd = elem.pCmds[0];
    beginCmd(cmd);

    RenderTargetBarrier barriers[]{
        {pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
    };
    cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, barriers);

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mRenderTargets[0].mClearValue = { 0.5f, 0.5f, 0.0f, 0.0f };
    bindRenderTargets.mRenderTargets[0].mOverrideClearValue = true;
    bindRenderTargets.mDepthStencil = { pRTDepth, LOAD_ACTION_CLEAR },
    cmdBindRenderTargets(cmd, &bindRenderTargets);

    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

    skybox.draw(pRenderTarget, cmd, gFrameIndex);

    const uint32_t stride = sizeof(float) * 3;
    cmdBindPipeline(cmd, pQuadPipeline);
    cmdBindDescriptorSet(cmd, 0, pQuadDS);
    cmdBindVertexBuffer(cmd, 1, &pQuadVB, &stride, NULL);
    cmdBindIndexBuffer(cmd, pQuadIB, INDEX_TYPE_UINT32, 0);
    cmdDrawIndexed(cmd, indexCount, 0, 0);

    barriers[0] = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
    cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, barriers);

    endCmd(cmd);

    QueueSubmitDesc submitDesc = {};
    submitDesc.ppCmds = &cmd;
    submitDesc.pSignalFence = elem.pFence;
    submitDesc.ppSignalSemaphores = &pRenderFinishedSemaphore;
    submitDesc.ppWaitSemaphores = &pImageAcquiredSemaphore;
    submitDesc.mCmdCount = 1;
    submitDesc.mWaitSemaphoreCount = 1;
    submitDesc.mSignalSemaphoreCount = 1;
    queueSubmit(pGraphicsQueue, &submitDesc);

    QueuePresentDesc presentDesc = {};
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.ppWaitSemaphores = &pRenderFinishedSemaphore;
    presentDesc.mWaitSemaphoreCount = 1;
    presentDesc.mIndex = static_cast<uint8_t>(swapchainImageIndex);
    presentDesc.mSubmitDone = true;
    queuePresent(pGraphicsQueue, &presentDesc);

    gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
}

void MainApp::Update(float deltaTime)
{
    updateInputSystem(deltaTime, mSettings.mWidth, mSettings.mHeight);

    Input::Update(deltaTime, (float)mSettings.mWidth, (float)mSettings.mHeight, viewMat, projMat);
    skybox.updateMatrices(projMat, viewMat);
    quadUBO.projectView = projMat * viewMat;
    quadUBO.color = vec3(1.0f, .0f, 0.0f);
    quadUBO.model = mat4::translation({ 0, 0, -200 }) * mat4::rotationX(degToRad(-90)) * mat4::scale({ 200, 200, 200 });

    BufferUpdateDesc quadUniformUpdateDesc = { pQuadUB };
    beginUpdateResource(&quadUniformUpdateDesc);
    *(QuadUBO*)quadUniformUpdateDesc.pMappedData = quadUBO;
    endUpdateResource(&quadUniformUpdateDesc);
}

const char* MainApp::GetName()
{
    return "The-Forge Playground";
}


void MainApp::Unload(ReloadDesc* pReloadDesc)
{
    waitQueueIdle(pGraphicsQueue);

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        skybox.detachPipeline(pRenderer);
        removePipeline(pRenderer, pQuadPipeline);
    }
    if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        skybox.unloadShaders(pRenderer);

        removeDescriptorSet(pRenderer, pQuadDS);
        removeDescriptorSet(pRenderer, pTextureDS);
        removeRootSignature(pRenderer, pQuadRS);
        removeShader(pRenderer, pQuadShader);
    }
    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
    {
        removeRenderTarget(pRenderer, pRTDepth);
        removeSwapChain(pRenderer, pSwapChain);
    }
}

void MainApp::Exit()
{
    skybox.cleanupBuffers(pRenderer);
    removeResource(pQuadUB);
    removeResource(pQuadVB);
    removeResource(pQuadIB);
    removeResource(pTexture);
    removeSampler(pRenderer, pSampler);

    Input::Cleanup();

    removeSemaphore(pRenderer, pImageAcquiredSemaphore);
    removeSemaphore(pRenderer, pRenderFinishedSemaphore);
    removeGpuCmdRing(pRenderer, &gGraphicsCmdRing);
    exitResourceLoaderInterface(pRenderer);

    removeQueue(pRenderer, pGraphicsQueue);
    exitRenderer(pRenderer);
    pRenderer = nullptr;
}

DEFINE_APPLICATION_MAIN(MainApp)