#ifndef SPHERE_H
#define SPHERE_H


struct UniformBlockSky
{
    CameraMatrix mProjectView;
};

class Sphere
{
public:
    void init(Renderer* pRenderer);
    void attachPipeline(Renderer* pRenderer, SwapChain* pSwwapchain);
    void prepareDescriptorSets(Renderer* pRenderer);
    void loadShaders(Renderer* pRenderer);

    void unloadShaders(Renderer* pRenderer);
    void detachPipeline(Renderer* pRenderer);
    void exit(Renderer* pRenderer);

    void updateMatrices(CameraMatrix projMat, mat4 viewMat);
    void updateBuffer(uint32_t frameIndex);
    void draw(RenderTarget* pRenderTarget, Cmd* cmd, uint32_t frameIndex);
    
private:

    int spherePoints = 0;

    struct SphereUniform
    {
        CameraMatrix projectView;
        CameraMatrix lightProjectView;
        mat4 world;
        vec4 color;
    } sphereUniform = {};


    RootSignature* pRSInstancing = nullptr;
    DescriptorSet* pDSSphereUniform = nullptr;
    Buffer* pBufferSphereUniform = nullptr;
    Buffer* pBufferSphereVertex = nullptr;
    Pipeline* pPipelineSphere = nullptr;
    Pipeline* pPipelineSphereShadow = nullptr;
};


void Sphere::init(Renderer* pRenderer)
{
    float* sphereVertices{};
    generateSpherePoints(&sphereVertices, &spherePoints, 12, 1.0f);

    uint64_t sphereDataSize = spherePoints * sizeof(float);
    BufferLoadDesc sphereVbDesc = {};
    sphereVbDesc.ppBuffer = &pBufferSphereVertex;
    sphereVbDesc.pData = sphereVertices;
    sphereVbDesc.mDesc = {};
    sphereVbDesc.mDesc.mSize = sphereDataSize;
    sphereVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    sphereVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;

    addResource(&sphereVbDesc, nullptr);

    BufferLoadDesc ubDesc = {};
    ubDesc.ppBuffer = &pBufferSphereUniform;
    ubDesc.mDesc = {};
    ubDesc.mDesc.mSize = sizeof(SphereUniform);
    ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    addResource(&ubDesc, nullptr);
}

void Sphere::attachPipeline(Renderer* pRenderer, SwapChain* pSwapChain)
{
    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    DepthStateDesc depthStateDesc = {};
    depthStateDesc.mDepthTest = true;
    depthStateDesc.mDepthWrite = true;
    depthStateDesc.mDepthFunc = CMP_GEQUAL;

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



void Sphere::prepareDescriptorSets(Renderer* pRenderer)
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

void Sphere::loadShaders(Renderer* pRenderer)
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






void Sphere::updateMatrices(CameraMatrix projMat, mat4 viewMat)
{
    viewMat.setTranslation(vec3(0));
    gUniformDataSky.mProjectView = projMat * viewMat;
}
void Sphere::updateBuffer(uint32_t frameIndex)
{
    BufferUpdateDesc skyboxViewProjCbv = { pSkyboxUniformBuffer[frameIndex] };
    beginUpdateResource(&skyboxViewProjCbv);
    memcpy(skyboxViewProjCbv.pMappedData, &gUniformDataSky, sizeof(gUniformDataSky));
    endUpdateResource(&skyboxViewProjCbv);
}
void Sphere::draw(RenderTarget* pRenderTarget, Cmd* cmd, uint32_t frameIndex)
{
    const uint32_t stride = sizeof(float) * 4;

    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 1.0f, 1.0f);
    cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
    cmdBindDescriptorSet(cmd, frameIndex * 2 + 0, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 1, &pSkyBoxVertexBuffer, &stride, NULL);
    cmdDraw(cmd, 36, 0);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
}



void Sphere::unloadShaders(Renderer* pRenderer)
{
    removeDescriptorSet(pRenderer, pDescriptorSetTexture);
    removeDescriptorSet(pRenderer, pDescriptorSetUniforms);
    removeRootSignature(pRenderer, pRootSignature);
    removeShader(pRenderer, pSkyBoxDrawShader);
}
void Sphere::detachPipeline(Renderer* pRenderer)
{
    removePipeline(pRenderer, pSkyBoxDrawPipeline);
}
void Sphere::exit(Renderer* pRenderer)
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


#endif // SPHERE_H
