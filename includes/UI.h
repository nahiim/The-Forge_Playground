#ifndef UI_H
#define UI_H


void reloadRequest(void*)
{
    ReloadDesc reload{ RELOAD_TYPE_SHADER };
    requestReload(&reload);
}

namespace UI
{
    uint32_t gFontID = 1;
    FontDrawDesc gFrameTimeDraw;

    //static unsigned char gPipelineStatsCharArray[2048] = {};
    //static bstring       gPipelineStats =bfromarr(gPipelineStatsCharArray);
    
    bool Init(Renderer* pRenderer);
    bool Load(SwapChain* pSwapChain, IApp::Settings settings, ReloadDesc* pReloadDesc);
    void UnLoad(ReloadDesc* pReloadDesc);
    void Draw(Cmd* cmd);
    void Draw(Cmd* cmd, ProfileToken gGpuProfileToken);
    void Cleanup();
}

bool UI::Init(Renderer* pRenderer)
{
    // Load fonts
    FontDesc font = {};
    font.pFontPath = "Crimson/Crimson-Italic.ttf";
    fntDefineFonts(&font, 1, &gFontID);

    FontSystemDesc fontRenderDesc = {};
    fontRenderDesc.pRenderer = pRenderer;
    if (!initFontSystem(&fontRenderDesc))
        return false; // report?

    // Initialize Forge User Interface Rendering
    UserInterfaceDesc uiRenderDesc = {};
    uiRenderDesc.pRenderer = pRenderer;
    initUserInterface(&uiRenderDesc);
    
    gFrameTimeDraw.mFontColor = 0xff00ffff;
    gFrameTimeDraw.mFontSize = 18.0f;
    gFrameTimeDraw.mFontID = gFontID;

    return true;
}

bool UI::Load(SwapChain* pSwapChain, IApp::Settings settings, ReloadDesc* pReloadDesc)
{
    UserInterfaceLoadDesc uiLoad = {};
    uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
    uiLoad.mHeight = settings.mHeight;
    uiLoad.mWidth = settings.mWidth;
    uiLoad.mLoadType = pReloadDesc->mType;
    loadUserInterface(&uiLoad);

    FontSystemLoadDesc fontLoad = {};
    fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
    fontLoad.mHeight = settings.mHeight;
    fontLoad.mWidth = settings.mWidth;
    fontLoad.mLoadType = pReloadDesc->mType;
    loadFontSystem(&fontLoad);

    return true;
}

void UI::UnLoad(ReloadDesc* pReloadDesc)
{
    unloadFontSystem(pReloadDesc->mType);
    unloadUserInterface(pReloadDesc->mType);
}



void UI::Draw(Cmd* cmd)
{
    cmdDrawUserInterface(cmd);
}

void UI::Draw(Cmd* cmd, ProfileToken gGpuProfileToken)
{
    float2 txtSizePx = cmdDrawCpuProfile(cmd, float2(8.f, 15.f), &gFrameTimeDraw);
    cmdDrawGpuProfile(cmd, float2(8.f, txtSizePx.y + 75.f), gGpuProfileToken, &gFrameTimeDraw);
}


void UI::Cleanup()
{
    exitUserInterface();
    exitFontSystem();
}


#endif // UI_H
