#include "MainApp.h"

std::vector<Particle> generateParticles(uint32_t width, uint32_t height, uint32_t count)
{
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(count);
    for (auto& particle : particles)
    {
        float r = 0.5f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
        float x = r * cos(theta) * height / width;
        float y = r * sin(theta);
        particle.position = vec2(x, y);
        particle.velocity = normalize(vec2(x, y)) * 0.00025f;
        particle.color = vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    return particles;
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
    addSemaphore(pRenderer, &pComputeFinishedSemaphore);

    initResourceLoaderInterface(pRenderer);

    particles = generateParticles(mSettings.mWidth, mSettings.mHeight, PARTICLE_COUNT);

    BufferLoadDesc SSBufferDesc = {};
    SSBufferDesc.ppBuffer = &pSSBuffer;
    SSBufferDesc.mDesc.mStructStride = sizeof(Particle);
    SSBufferDesc.mDesc.mElementCount = particles.size();
    SSBufferDesc.pData = particles.data();
    SSBufferDesc.mDesc.mSize = particles.size() * sizeof(Particle);
    SSBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    SSBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_RW_BUFFER;
    addResource(&SSBufferDesc, nullptr);

    BufferLoadDesc uniformBufferDesc = {};
    uniformBufferDesc.ppBuffer = &pUniformBuffer;
    uniformBufferDesc.mDesc.mSize = sizeof(ubo);
    uniformBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    uniformBufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    uniformBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    addResource(&uniformBufferDesc, nullptr);

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
    }

    if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        ShaderLoadDesc shaderDesc{};
        shaderDesc.mStages[0].pFileName = "particle.vert";
        shaderDesc.mStages[1].pFileName = "particle.frag";
        addShader(pRenderer, &shaderDesc, &pRasterShader);

        RootSignatureDesc rootDesc{};
        rootDesc.ppShaders = &pRasterShader;
        rootDesc.mShaderCount = 1;
        addRootSignature(pRenderer, &rootDesc, &pRasterRootSignature);

        ShaderLoadDesc computeShaderDesc = {};
        computeShaderDesc.mStages[0].pFileName = "particle.comp";
        addShader(pRenderer, &computeShaderDesc, &pComputeShader);

        RootSignatureDesc computeRootDesc{};
        computeRootDesc.ppShaders = &pComputeShader;
        computeRootDesc.mShaderCount = 1;
        addRootSignature(pRenderer, &computeRootDesc, &pComputeRootSignature);

        DescriptorSetDesc dsSSBDesc;
        dsSSBDesc.mMaxSets = 1;
        dsSSBDesc.mUpdateFrequency = DESCRIPTOR_UPDATE_FREQ_NONE;
        dsSSBDesc.pRootSignature = pComputeRootSignature;
        addDescriptorSet(pRenderer, &dsSSBDesc, &pDescriptorSet);
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
    {
        RenderTargetDesc desc{};
        desc.mFlags = TEXTURE_CREATION_FLAG_ON_TILE | TEXTURE_CREATION_FLAG_VR_MULTIVIEW;
        desc.mWidth = pSwapChain->ppRenderTargets[0]->mWidth;
        desc.mHeight = pSwapChain->ppRenderTargets[0]->mHeight;
        desc.mDepth = 1;
        desc.mArraySize = 1;
        desc.mSampleCount = SAMPLE_COUNT_1;
        desc.mFormat = depthBufferFormat;
        desc.mStartState = RESOURCE_STATE_DEPTH_WRITE;
        desc.mSampleQuality = 0;
        addRenderTarget(pRenderer, &desc, &pRTDepth);
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        RasterizerStateDesc rasterizerStateDesc = {};
        rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
        DepthStateDesc depthStateDesc = {};
        depthStateDesc.mDepthTest = true;
        depthStateDesc.mDepthWrite = true;
        depthStateDesc.mDepthFunc = CMP_GEQUAL;

        PipelineDesc computeDesc = {};
        computeDesc.mType = PIPELINE_TYPE_COMPUTE;
        ComputePipelineDesc& cpipelineSettings = computeDesc.mComputeDesc;
        cpipelineSettings.pShaderProgram = pComputeShader;
        cpipelineSettings.pRootSignature = pComputeRootSignature;
        addPipeline(pRenderer, &computeDesc, &pComputePipeline);
        
        VertexLayout vertexLayout = {};
        vertexLayout.mBindingCount = 1;
        vertexLayout.mBindings[0].mStride = sizeof(Particle);

        vertexLayout.mAttribCount = 3;
        vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32_SFLOAT;
        vertexLayout.mAttribs[0].mBinding = 0;
        vertexLayout.mAttribs[0].mLocation = 0;
        vertexLayout.mAttribs[0].mOffset = 0;

        vertexLayout.mAttribs[1].mSemantic = SEMANTIC_CUSTOM;
        vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32_SFLOAT;
        vertexLayout.mAttribs[1].mBinding = 0;
        vertexLayout.mAttribs[1].mLocation = 1;
        vertexLayout.mAttribs[1].mOffset = offsetof(Particle, velocity);

        vertexLayout.mAttribs[2].mSemantic = SEMANTIC_COLOR;
        vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
        vertexLayout.mAttribs[2].mBinding = 0;
        vertexLayout.mAttribs[2].mLocation = 2;
        vertexLayout.mAttribs[2].mOffset = offsetof(Particle, color);

        PipelineDesc graphicsDesc = {};
        graphicsDesc.mType = PIPELINE_TYPE_GRAPHICS;
        graphicsDesc.mGraphicsDesc = {};
        graphicsDesc.mGraphicsDesc.pShaderProgram = pRasterShader;
        graphicsDesc.mGraphicsDesc.pRootSignature = pRasterRootSignature;
        graphicsDesc.mGraphicsDesc.pVertexLayout = &vertexLayout;
        graphicsDesc.mGraphicsDesc.pDepthState = &depthStateDesc;
        graphicsDesc.mGraphicsDesc.pRasterizerState = &rasterizerStateDesc;
        graphicsDesc.mGraphicsDesc.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        graphicsDesc.mGraphicsDesc.mRenderTargetCount = 1;
        graphicsDesc.mGraphicsDesc.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        graphicsDesc.mGraphicsDesc.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        graphicsDesc.mGraphicsDesc.mDepthStencilFormat = TinyImageFormat_D32_SFLOAT;
        graphicsDesc.mGraphicsDesc.mPrimitiveTopo = PRIMITIVE_TOPO_POINT_LIST;
        graphicsDesc.mGraphicsDesc.mVRFoveatedRendering = true;
        addPipeline(pRenderer, &graphicsDesc, &pGraphicsPipeline);
    }

    // Prepare Descriptore sets
    DescriptorData params[3] = {};
    params[0].pName = "ubo";
    params[0].ppBuffers = &pUniformBuffer;
    params[1].pName = "ParticleSSBOIn";
    params[1].ppBuffers = &pSSBuffer;
    params[2].pName = "ParticleSSBOOut";
    params[2].ppBuffers = &pSSBuffer;
    updateDescriptorSet(pRenderer, 0, pDescriptorSet, 3, params);

    waitForAllResourceLoads();

	return true;
}


void MainApp::Draw()
{
    GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);

    // Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
    {
        waitForFences(pRenderer, 1, &elem.pFence);
    }

    Cmd* cmd = elem.pCmds[0];
    beginCmd(cmd);

    cmdBindPipeline(cmd, pComputePipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSet);
    cmdDispatch(cmd, PARTICLE_COUNT / 256, 1, 1);

    endCmd(cmd);
    
    QueueSubmitDesc submitDispatchDesc = {};
    submitDispatchDesc.ppCmds = &cmd;
    submitDispatchDesc.pSignalFence = elem.pFence;
    submitDispatchDesc.ppSignalSemaphores = &pComputeFinishedSemaphore;
    submitDispatchDesc.mCmdCount = 1;
    submitDispatchDesc.mSignalSemaphoreCount = 1;
    queueSubmit(pGraphicsQueue, &submitDispatchDesc);
    

    if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
    {
        waitQueueIdle(pGraphicsQueue);
        ::toggleVSync(pRenderer, &pSwapChain);
    }
    uint32_t swapchainImageIndex;
    acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, nullptr, &swapchainImageIndex);

    RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];

    // Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
    getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
    {
        waitForFences(pRenderer, 1, &elem.pFence);
    }

    // Reset cmd pool for this frame
    resetCmdPool(pRenderer, elem.pCmdPool);

    cmd = elem.pCmds[0];
    beginCmd(cmd);

    RenderTargetBarrier barriers[]{
        {pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
    };
    cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, barriers);

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mRenderTargets[0].mClearValue = { 0.4f, 0.4f, 0.2f, 0.0f };
    bindRenderTargets.mRenderTargets[0].mOverrideClearValue = true;
    bindRenderTargets.mDepthStencil = { pRTDepth, LOAD_ACTION_CLEAR },
    cmdBindRenderTargets(cmd, &bindRenderTargets);

    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

    cmdBindPipeline(cmd, pGraphicsPipeline);
    cmdBindVertexBuffer(cmd, 1, &pSSBuffer, &particle_stride, nullptr);
    cmdDraw(cmd, particles.size(), 0);
    
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
//    updateInputSystem(deltaTime, mSettings.mWidth, mSettings.mHeight);
//    Input::Update(deltaTime, (float)mSettings.mWidth, (float)mSettings.mHeight, viewMat, projMat);
//    std::cout << "\n\nDELTA- " << deltaTime << " seconds\n\n";

    ubo.delta_time = deltaTime * 2000;

    BufferUpdateDesc uniformUpdateDesc = { pUniformBuffer};
    beginUpdateResource(&uniformUpdateDesc);
    *(UBO*)uniformUpdateDesc.pMappedData = ubo;
    endUpdateResource(&uniformUpdateDesc);
}

const char* MainApp::GetName()
{
	return "The-Forge - Particles(Compute)";
}


void MainApp::Unload(ReloadDesc* pReloadDesc)
{
    waitQueueIdle(pGraphicsQueue);

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        removePipeline(pRenderer, pComputePipeline);
        removePipeline(pRenderer, pGraphicsPipeline);
    }
    if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        removeDescriptorSet(pRenderer, pDescriptorSet);

        removeRootSignature(pRenderer, pRasterRootSignature);
        removeRootSignature(pRenderer, pComputeRootSignature);
        removeShader(pRenderer, pComputeShader);
        removeShader(pRenderer, pRasterShader);
    }
    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
    {
        removeRenderTarget(pRenderer, pRTDepth);
        removeSwapChain(pRenderer, pSwapChain);
    }
}

void MainApp::Exit()
{
    removeResource(pSSBuffer);
    removeResource(pUniformBuffer);

    Input::Cleanup();

    removeSemaphore(pRenderer, pImageAcquiredSemaphore);
    removeSemaphore(pRenderer, pRenderFinishedSemaphore);
    removeSemaphore(pRenderer, pComputeFinishedSemaphore);
    removeGpuCmdRing(pRenderer, &gGraphicsCmdRing);
    exitResourceLoaderInterface(pRenderer);

    removeQueue(pRenderer, pGraphicsQueue);
    exitRenderer(pRenderer);
    pRenderer = nullptr;
}


DEFINE_APPLICATION_MAIN(MainApp)