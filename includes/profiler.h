#ifndef PROFILER_H
#define PROFILER_H

namespace Profiler
{
    ProfileToken gGpuProfileToken = PROFILE_INVALID_TOKEN;

    void Init(Renderer* pRenderer, Queue* pGraphicsQueue, uint32_t width, uint32_t height);
    void Cleanup();
}

void Profiler::Init(Renderer* pRenderer, Queue* pGraphicsQueue, uint32_t width, uint32_t height)
{
    // Initialize micro profiler and its UI.
    ProfilerDesc profiler = {};
    profiler.pRenderer = pRenderer;
    profiler.mWidthUI = static_cast<uint32_t>(width);
    profiler.mHeightUI = static_cast<uint32_t>(height);

    initProfiler(&profiler);

    // Gpu profiler can only be added after initProfile.
    gGpuProfileToken = addGpuProfiler(pRenderer, pGraphicsQueue, "Graphics");
}

void Profiler::Cleanup()
{
    exitProfiler();
}


#endif // PROFILER_H
