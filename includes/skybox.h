#ifndef SKYBOX_H
#define SKYBOX_H


struct UniformBlockSky
{
    CameraMatrix mProjectView;
};

class Skybox
{
public:
    void init(Renderer* pRenderer);
    void attachPipeline(Renderer* pRenderer, SwapChain* pSwwapchain, RasterizerStateDesc rasterizerStateDesc, DepthStateDesc depthStateDesc);
    void prepareDescriptorSets(Renderer* pRenderer);
    void loadShaders(Renderer* pRenderer);

    void unloadShaders(Renderer* pRenderer);
    void detachPipeline(Renderer* pRenderer);
    void cleanupBuffers(Renderer* pRenderer);

    void updateMatrices(CameraMatrix projMat, mat4 viewMat);
    void updateBuffer(uint32_t frameIndex);
    void draw(RenderTarget* pRenderTarget, Cmd* cmd, uint32_t frameIndex);
    
private:

    Shader* pSkyBoxDrawShader = NULL;
    Buffer* pSkyBoxVertexBuffer = NULL;
    Pipeline* pSkyBoxDrawPipeline = NULL;
    RootSignature* pRootSignature = NULL;
    Sampler* pSamplerSkyBox = NULL;
    Texture* pSkyBoxTextures[6];
    DescriptorSet* pDescriptorSetTexture = { NULL };
    DescriptorSet* pDescriptorSetUniforms = { NULL };

    UniformBlockSky  gUniformDataSky;
    Buffer* pSkyboxUniformBuffer[gDataBufferCount] = { NULL };


    const char* mSkyBoxImageFileNames[6] = {
        "Skybox_right1.tex", "Skybox_left2.tex", "Skybox_top3.tex",
        "Skybox_bottom4.tex", "Skybox_front5.tex", "Skybox_back6.tex"
    };
    const float mSkyBoxPoints[144] = {
        10.0f,  -10.0f, -10.0f, 6.0f, // -z
        -10.0f, -10.0f, -10.0f, 6.0f,   -10.0f, 10.0f,  -10.0f, 6.0f,   -10.0f, 10.0f,
        -10.0f, 6.0f,   10.0f,  10.0f,  -10.0f, 6.0f,   10.0f,  -10.0f, -10.0f, 6.0f,

        -10.0f, -10.0f, 10.0f,  2.0f, //-x
        -10.0f, -10.0f, -10.0f, 2.0f,   -10.0f, 10.0f,  -10.0f, 2.0f,   -10.0f, 10.0f,
        -10.0f, 2.0f,   -10.0f, 10.0f,  10.0f,  2.0f,   -10.0f, -10.0f, 10.0f,  2.0f,

        10.0f,  -10.0f, -10.0f, 1.0f, //+x
        10.0f,  -10.0f, 10.0f,  1.0f,   10.0f,  10.0f,  10.0f,  1.0f,   10.0f,  10.0f,
        10.0f,  1.0f,   10.0f,  10.0f,  -10.0f, 1.0f,   10.0f,  -10.0f, -10.0f, 1.0f,

        -10.0f, -10.0f, 10.0f,  5.0f, // +z
        -10.0f, 10.0f,  10.0f,  5.0f,   10.0f,  10.0f,  10.0f,  5.0f,   10.0f,  10.0f,
        10.0f,  5.0f,   10.0f,  -10.0f, 10.0f,  5.0f,   -10.0f, -10.0f, 10.0f,  5.0f,

        -10.0f, 10.0f,  -10.0f, 3.0f, //+y
        10.0f,  10.0f,  -10.0f, 3.0f,   10.0f,  10.0f,  10.0f,  3.0f,   10.0f,  10.0f,
        10.0f,  3.0f,   -10.0f, 10.0f,  10.0f,  3.0f,   -10.0f, 10.0f,  -10.0f, 3.0f,

        10.0f,  -10.0f, 10.0f,  4.0f, //-y
        10.0f,  -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f,
        -10.0f, 4.0f,   -10.0f, -10.0f, 10.0f,  4.0f,   10.0f,  -10.0f, 10.0f,  4.0f,
    };
};


void Skybox::init(Renderer* pRenderer)
{
    gUniformDataSky = {};

    // Loads Skybox Textures
    for (int i = 0; i < 6; ++i)
    {
        TextureLoadDesc textureDesc = {};
        textureDesc.pFileName = mSkyBoxImageFileNames[i];
        textureDesc.ppTexture = &pSkyBoxTextures[i];
        // Textures representing color should be stored in SRGB or HDR format
        textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
        addResource(&textureDesc, NULL);
    }

    // Dynamic sampler that is bound at runtime
    SamplerDesc samplerDesc = { FILTER_LINEAR,
                                FILTER_LINEAR,
                                MIPMAP_MODE_NEAREST,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE,
                                ADDRESS_MODE_CLAMP_TO_EDGE };
    addSampler(pRenderer, &samplerDesc, &pSamplerSkyBox);

    uint64_t       skyBoxDataSize = 4 * 6 * 6 * sizeof(float);
    BufferLoadDesc skyboxVbDesc = {};
    skyboxVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    skyboxVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    skyboxVbDesc.mDesc.mSize = skyBoxDataSize;
    skyboxVbDesc.pData = mSkyBoxPoints;
    skyboxVbDesc.ppBuffer = &pSkyBoxVertexBuffer;
    addResource(&skyboxVbDesc, NULL);

    BufferLoadDesc ubDesc = {};
    ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    ubDesc.pData = NULL;
    for (uint32_t i = 0; i < gDataBufferCount; ++i)
    {
        ubDesc.mDesc.pName = "SkyboxUniformBuffer";
        ubDesc.mDesc.mSize = sizeof(UniformBlockSky);
        ubDesc.ppBuffer = &pSkyboxUniformBuffer[i];
        addResource(&ubDesc, NULL);
    }
}

void Skybox::attachPipeline(Renderer* pRenderer, SwapChain* pSwapChain, RasterizerStateDesc rasterizerStateDesc, DepthStateDesc depthStateDesc)
{
    PipelineDesc desc = {};
    desc.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
    pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    pipelineSettings.mRenderTargetCount = 1;

    // layout and pipeline for skybox draw
    VertexLayout vertexLayout = {};
    vertexLayout.mBindingCount = 1;
    vertexLayout.mBindings[0].mStride = sizeof(float4);
    vertexLayout.mAttribCount = 1;
    vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
    vertexLayout.mAttribs[0].mBinding = 0;
    vertexLayout.mAttribs[0].mLocation = 0;
    vertexLayout.mAttribs[0].mOffset = 0;
    pipelineSettings.pVertexLayout = &vertexLayout;

    pipelineSettings.pDepthState = &depthStateDesc;
    pipelineSettings.mDepthStencilFormat = TinyImageFormat_D32_SFLOAT;
    pipelineSettings.mVRFoveatedRendering = true;
    pipelineSettings.pRootSignature = pRootSignature;
    pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
    pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
    pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
    pipelineSettings.pDepthState = NULL;
    pipelineSettings.pRasterizerState = &rasterizerStateDesc;
    pipelineSettings.pShaderProgram = pSkyBoxDrawShader; //-V519
    addPipeline(pRenderer, &desc, &pSkyBoxDrawPipeline);
}



void Skybox::prepareDescriptorSets(Renderer* pRenderer)
{
    // Prepare descriptor sets
    DescriptorData params[7] = {};
    params[0].pName = "RightText";
    params[0].ppTextures = &pSkyBoxTextures[0];
    params[1].pName = "LeftText";
    params[1].ppTextures = &pSkyBoxTextures[1];
    params[2].pName = "TopText";
    params[2].ppTextures = &pSkyBoxTextures[2];
    params[3].pName = "BotText";
    params[3].ppTextures = &pSkyBoxTextures[3];
    params[4].pName = "FrontText";
    params[4].ppTextures = &pSkyBoxTextures[4];
    params[5].pName = "BackText";
    params[5].ppTextures = &pSkyBoxTextures[5];
    params[6].pName = "uSampler0";
    params[6].ppSamplers = &pSamplerSkyBox;
    updateDescriptorSet(pRenderer, 0, pDescriptorSetTexture, 7, params);

    for (uint32_t i = 0; i < gDataBufferCount; ++i)
    {
        DescriptorData params[1] = {};
        params[0].pName = "uniformBlock";
        params[0].ppBuffers = &pSkyboxUniformBuffer[i];
        updateDescriptorSet(pRenderer, i * 2 + 0, pDescriptorSetUniforms, 1, params);
    }
}

void Skybox::loadShaders(Renderer* pRenderer)
{
    ShaderLoadDesc skyShader = {};
    skyShader.mStages[0].pFileName = "skybox.vert";
    skyShader.mStages[1].pFileName = "skybox.frag";
    addShader(pRenderer, &skyShader, &pSkyBoxDrawShader);

    Shader* shader = pSkyBoxDrawShader;
    RootSignatureDesc rootDesc = {};
    rootDesc.mShaderCount = 1;
    rootDesc.ppShaders = &shader;
    addRootSignature(pRenderer, &rootDesc, &pRootSignature);

    DescriptorSetDesc desc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
    addDescriptorSet(pRenderer, &desc, &pDescriptorSetTexture);
    desc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount * 2 };
    addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);
}






void Skybox::updateMatrices(CameraMatrix projMat, mat4 viewMat)
{
    viewMat.setTranslation(vec3(0));
    gUniformDataSky.mProjectView = projMat * viewMat;
}
void Skybox::updateBuffer(uint32_t frameIndex)
{
    BufferUpdateDesc skyboxViewProjCbv = { pSkyboxUniformBuffer[frameIndex] };
    beginUpdateResource(&skyboxViewProjCbv);
    memcpy(skyboxViewProjCbv.pMappedData, &gUniformDataSky, sizeof(gUniformDataSky));
    endUpdateResource(&skyboxViewProjCbv);
}
void Skybox::draw(RenderTarget* pRenderTarget, Cmd* cmd, uint32_t frameIndex)
{
    const uint32_t stride = sizeof(float) * 4;

//    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 1.0f, 1.0f);
    cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
    cmdBindDescriptorSet(cmd, frameIndex * 2 + 0, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 1, &pSkyBoxVertexBuffer, &stride, NULL);
    cmdDraw(cmd, 36, 0);
//    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
}



void Skybox::unloadShaders(Renderer* pRenderer)
{
    removeDescriptorSet(pRenderer, pDescriptorSetTexture);
    removeDescriptorSet(pRenderer, pDescriptorSetUniforms);
    removeRootSignature(pRenderer, pRootSignature);
    removeShader(pRenderer, pSkyBoxDrawShader);
}
void Skybox::detachPipeline(Renderer* pRenderer)
{
    removePipeline(pRenderer, pSkyBoxDrawPipeline);
}
void Skybox::cleanupBuffers(Renderer* pRenderer)
{
    for (uint32_t i = 0; i < gDataBufferCount; ++i)
    {
        removeResource(pSkyboxUniformBuffer[i]);
    }

    removeResource(pSkyBoxVertexBuffer);

    for (uint i = 0; i < 6; ++i)
        removeResource(pSkyBoxTextures[i]);

    removeSampler(pRenderer, pSamplerSkyBox);
}


#endif // SKYBOX_H
