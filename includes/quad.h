#ifndef QUAD_H
#define QUAD_H


class Quad
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
    void updateBuffer();
    void draw(RenderTarget* pRenderTarget, Cmd* cmd);
    
private:

    int quadPoints = 0;
    struct QuadUniform
    {
        CameraMatrix projectView;
        CameraMatrix lightProjectView;
        mat4 world;
        vec4 color;
    } quadUniform = {};

    Shader* pShaderQuad = nullptr;
    RootSignature* pRSQuad = nullptr;
    DescriptorSet* pDSQuadUniform = nullptr;
    Buffer* pBufferQuadVertex = nullptr;
    Buffer* pBufferQuadIndex = nullptr;
    Buffer* pBufferQuadUniform = nullptr;
    Pipeline* pPipelineQuad = nullptr;
};


void Quad::init(Renderer* pRenderer)
{
    float* quadVertices{};
    generateQuad(&quadVertices, &quadPoints);

    uint16_t quadIndices[6] = { 0, 1, 2, 1, 3, 2 };
    uint64_t quadDataSize = quadPoints * sizeof(float);

    BufferLoadDesc quadVbDesc = {};
    quadVbDesc.ppBuffer = &pBufferQuadVertex;
    quadVbDesc.pData = quadVertices;
    quadVbDesc.mDesc = {};
    quadVbDesc.mDesc.mSize = quadDataSize;
    quadVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    quadVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    addResource(&quadVbDesc, nullptr);

    BufferLoadDesc quadIdDesc = {};
    quadIdDesc.ppBuffer = &pBufferQuadIndex;
    quadIdDesc.pData = quadIndices;
    quadIdDesc.mDesc = {};
    quadIdDesc.mDesc.mSize = sizeof(uint16_t) * 6;
    quadIdDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    quadIdDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
    addResource(&quadIdDesc, nullptr);

    BufferLoadDesc quadUniformDesc = {};
    quadUniformDesc.ppBuffer = &pBufferQuadUniform;
    quadUniformDesc.mDesc = {};
    quadUniformDesc.mDesc.mSize = sizeof(QuadUniform);
    quadUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    quadUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    quadUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    addResource(&quadUniformDesc, nullptr);

    waitForAllResourceLoads();

    tf_free(quadVertices);
}

void Quad::attachPipeline(Renderer* pRenderer, SwapChain* pSwapChain, RasterizerStateDesc rasterizerStateDesc, DepthStateDesc depthStateDesc)
{
    // layout and pipeline
    VertexLayout vertexLayout = {};
    vertexLayout.mBindingCount = 1;
    vertexLayout.mBindings[0].mStride = sizeof(float) * 6;

    vertexLayout.mAttribCount = 2;
    vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.mAttribs[0].mBinding = 0;
    vertexLayout.mAttribs[0].mLocation = 0;
    vertexLayout.mAttribs[0].mOffset = 0;

    vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
    vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.mAttribs[1].mBinding = 0;
    vertexLayout.mAttribs[1].mLocation = 1;
    vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);


    PipelineDesc desc = {};
    desc.mType = PIPELINE_TYPE_GRAPHICS;
    desc.mGraphicsDesc = {};
    desc.mGraphicsDesc.pShaderProgram = pShaderQuad;
    desc.mGraphicsDesc.pRootSignature = pRSQuad;
    desc.mGraphicsDesc.pVertexLayout = &vertexLayout;
    desc.mGraphicsDesc.pDepthState = &depthStateDesc;
    desc.mGraphicsDesc.pRasterizerState = &rasterizerStateDesc;
    desc.mGraphicsDesc.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
    desc.mGraphicsDesc.mRenderTargetCount = 1;
    desc.mGraphicsDesc.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
    desc.mGraphicsDesc.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
    desc.mGraphicsDesc.mDepthStencilFormat = TinyImageFormat_D32_SFLOAT;
    desc.mGraphicsDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    desc.mGraphicsDesc.mVRFoveatedRendering = true;
    addPipeline(pRenderer, &desc, &pPipelineQuad);
    ASSERT(pPipelineQuad);
}



void Quad::prepareDescriptorSets(Renderer* pRenderer)
{
    DescriptorData params = {};
    params.pName = "uniformBlock";
    params.ppBuffers = &pBufferQuadUniform;
    updateDescriptorSet(pRenderer, 0, pDSQuadUniform, 1, &params);
}
void Quad::loadShaders(Renderer* pRenderer)
{
    ShaderLoadDesc shaderDesc{};
    shaderDesc.mStages[0].pFileName = "quad.vert";
    shaderDesc.mStages[1].pFileName = "basic.frag";
    addShader(pRenderer, &shaderDesc, &pShaderQuad);
    ASSERT(pShaderQuad);

    RootSignatureDesc rootDesc{};
    rootDesc.ppShaders = &pShaderQuad;
    rootDesc.mShaderCount = 1;
    addRootSignature(pRenderer, &rootDesc, &pRSQuad);
    ASSERT(pRSQuad);

    DescriptorSetDesc dsDesc = { pRSQuad, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, 1 };
    addDescriptorSet(pRenderer, &dsDesc, &pDSQuadUniform);
    ASSERT(pDSQuadUniform);
}



void Quad::updateMatrices(CameraMatrix projMat, mat4 viewMat)
{
    quadUniform.projectView = projMat * viewMat;
    quadUniform.color = { 1.0f, 1.0f, .0f, 1.0f };
    quadUniform.world = mat4::translation({ 0, -200, 0 }) * mat4::rotationX(degToRad(-90)) * mat4::scale({ 200, 200, 200 });
}
void Quad::updateBuffer()
{
    BufferUpdateDesc quadUniformUpdateDesc = { pBufferQuadUniform };
    beginUpdateResource(&quadUniformUpdateDesc);
    *(QuadUniform*)quadUniformUpdateDesc.pMappedData = quadUniform;
    endUpdateResource(&quadUniformUpdateDesc);
}
void Quad::draw(RenderTarget* pRenderTarget, Cmd* cmd)
{
    constexpr uint32_t stride = sizeof(float) * 6;
    cmdBindPipeline(cmd, pPipelineQuad);
    cmdBindDescriptorSet(cmd, 0, pDSQuadUniform);
    cmdBindVertexBuffer(cmd, 1, &pBufferQuadVertex, &stride, nullptr);
    cmdBindIndexBuffer(cmd, pBufferQuadIndex, INDEX_TYPE_UINT16, 0);
    cmdDrawIndexed(cmd, 6, 0, 0);
}



void Quad::detachPipeline(Renderer* pRenderer)
{
    removePipeline(pRenderer, pPipelineQuad);
}
void Quad::unloadShaders(Renderer* pRenderer)
{
    removeDescriptorSet(pRenderer, pDSQuadUniform);
    removeRootSignature(pRenderer, pRSQuad);
    removeShader(pRenderer, pShaderQuad);
}
void Quad::cleanupBuffers(Renderer* pRenderer)
{
    removeResource(pBufferQuadUniform);
    removeResource(pBufferQuadVertex);
    removeResource(pBufferQuadIndex);
}
#endif // QUAD_H