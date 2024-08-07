
#include "MainApp.h"


bool MainApp::Init()
{
    extern PlatformParameters gPlatformParameters;
    gPlatformParameters.mSelectedRendererApi = RendererApi::RENDERER_API_VULKAN;

    // FILE PATHS
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "CompiledShaders");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");
    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_DEBUG, "Debug");
    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SCREENSHOTS, "Screenshots");

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

    initResourceLoaderInterface(pRenderer);
    skybox.init(pRenderer);
    quad.init(pRenderer);

    Profiler::Init(pRenderer, pGraphicsQueue, mSettings.mWidth, mSettings.mHeight);
    UI::Init(pRenderer);

    /************************************************************************/
    // GUI
    /************************************************************************/    
    UIComponentDesc qwe = {};
    qwe.mStartPosition = vec2(mSettings.mWidth * 0.01f, mSettings.mHeight * 0.2f);
    qwe.mStartSize = vec2(1000.0f, 1000.0f);
    uiCreateComponent("Some Text", &qwe, &pGuiWindow);

    static float4     color = { 1.0f, .0f, 1.0f, 1.0f };
    DynamicTextWidget textWidget;
    textWidget.pText = &someText;
    textWidget.pColor = &color;
    uiCreateComponentWidget(pGuiWindow, "SOME TEXT", &textWidget, WIDGET_TYPE_DYNAMIC_TEXT);

    // Take a screenshot with a button.
    ButtonWidget screenshot{};
    UIWidget* pScreenshot = uiCreateComponentWidget(pGuiWindow, "Screenshot", &screenshot, WIDGET_TYPE_BUTTON);

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
        quad.loadShaders(pRenderer);
        skybox.loadShaders(pRenderer);

        RasterizerStateDesc rasterizerStateDesc = {};
        rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
        DepthStateDesc depthStateDesc = {};
        depthStateDesc.mDepthTest = true;
        depthStateDesc.mDepthWrite = true;
        depthStateDesc.mDepthFunc = CMP_GEQUAL;

        quad.attachPipeline(pRenderer, pSwapChain, rasterizerStateDesc, depthStateDesc);
        skybox.attachPipeline(pRenderer, pSwapChain, rasterizerStateDesc, depthStateDesc);
    }

    quad.prepareDescriptorSets(pRenderer);
    skybox.prepareDescriptorSets(pRenderer);

    UI::Load(pSwapChain, mSettings, pReloadDesc);

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

    quad.updateBuffer();
    skybox.updateBuffer(gFrameIndex);

    // Reset cmd pool for this frame
    resetCmdPool(pRenderer, elem.pCmdPool);

    Cmd* cmd = elem.pCmds[0];
    beginCmd(cmd);
    cmdBeginGpuFrameProfile(cmd, Profiler::gGpuProfileToken);

    RenderTargetBarrier barriers[]{
        {pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
    };
    cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, barriers);


    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mDepthStencil = { pRTDepth, LOAD_ACTION_CLEAR },
    cmdBindRenderTargets(cmd, &bindRenderTargets);

    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);
    
    cmdBeginGpuTimestampQuery(cmd, Profiler::gGpuProfileToken, "Draw Scene");
    skybox.draw(pRenderTarget, cmd, gFrameIndex);
    cmdEndGpuTimestampQuery(cmd, Profiler::gGpuProfileToken);

    cmdBeginGpuTimestampQuery(cmd, Profiler::gGpuProfileToken, "Draw quad");
    quad.draw(pRenderTarget, cmd);
    cmdEndGpuTimestampQuery(cmd, Profiler::gGpuProfileToken);


    UI::Draw(cmd, Profiler::gGpuProfileToken);
    UI::Draw(cmd);

    cmdBindRenderTargets(cmd, nullptr);

    bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_LOAD };
    bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
    cmdBindRenderTargets(cmd, &bindRenderTargets);

    barriers[0] = {
        pRenderTarget,
        RESOURCE_STATE_RENDER_TARGET,
        RESOURCE_STATE_PRESENT,
    };
    cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, barriers);

    cmdEndGpuFrameProfile(cmd, Profiler::gGpuProfileToken);
    endCmd(cmd);

    FlushResourceUpdateDesc flushUpdateDesc = {};
    flushUpdateDesc.mNodeIndex = 0;
    flushResourceUpdates(&flushUpdateDesc);

    Semaphore* waitSemaphores[2] = {
        flushUpdateDesc.pOutSubmittedSemaphore,
        pImageAcquiredSemaphore,
    };

    QueueSubmitDesc submitDesc = {};
    submitDesc.ppCmds = &cmd;
    submitDesc.pSignalFence = elem.pFence;
    submitDesc.ppWaitSemaphores = waitSemaphores;
    submitDesc.ppSignalSemaphores = &elem.pSemaphore;
    submitDesc.mCmdCount = 1;
    submitDesc.mWaitSemaphoreCount = 2;
    submitDesc.mSignalSemaphoreCount = 1;
    queueSubmit(pGraphicsQueue, &submitDesc);

    QueuePresentDesc presentDesc = {};
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.ppWaitSemaphores = waitSemaphores;
    presentDesc.mWaitSemaphoreCount = 2;
    presentDesc.mIndex = static_cast<uint8_t>(swapchainImageIndex);
    presentDesc.mSubmitDone = true;
    queuePresent(pGraphicsQueue, &presentDesc);

    flipProfiler();

    gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
}

void MainApp::Update(float deltaTime)
{
    updateInputSystem(deltaTime, mSettings.mWidth, mSettings.mHeight);

    Input::Update(deltaTime, (float)mSettings.mWidth, (float)mSettings.mHeight, viewMat, projMat);
    skybox.updateMatrices(projMat, viewMat);
    quad.updateMatrices(projMat, viewMat);
}

const char* MainApp::GetName()
{
	return "The-Forge Playground";
}


void MainApp::Unload(ReloadDesc* pReloadDesc)
{
    waitQueueIdle(pGraphicsQueue);

    UI::UnLoad(pReloadDesc);

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        quad.detachPipeline(pRenderer);
        skybox.detachPipeline(pRenderer);
    }
    if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        quad.unloadShaders(pRenderer);
        skybox.unloadShaders(pRenderer);
    }
    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
    {
        removeRenderTarget(pRenderer, pRTDepth);
        removeSwapChain(pRenderer, pSwapChain);
    }
}

void MainApp::Exit()
{
    quad.cleanupBuffers(pRenderer);
    skybox.cleanupBuffers(pRenderer);

    Input::Cleanup();
    UI::Cleanup();
    Profiler::Cleanup();
//    exitScreenshotInterface();

    removeSemaphore(pRenderer, pImageAcquiredSemaphore);
    removeGpuCmdRing(pRenderer, &gGraphicsCmdRing);
    exitResourceLoaderInterface(pRenderer);

    removeQueue(pRenderer, pGraphicsQueue);
    exitRenderer(pRenderer);
    pRenderer = nullptr;
}

DEFINE_APPLICATION_MAIN(MainApp)