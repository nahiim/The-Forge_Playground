
@echo off
setlocal

:: Define the shader directory
set SHADER_DIR=shaders/compiled

set OUTPUT_DIR=x64/output/Debug/CompiledShaders/VULKAN

for %%f in (shaders\*.vert) do (
    copy "%%f" "%SHADER_DIR%"
)
for %%f in (shaders\*.frag) do (
    copy "%%f" "%SHADER_DIR%"
)
for %%f in (shaders\*.comp) do (
    copy "%%f" "%SHADER_DIR%"
)

:: Compile shaders
for %%f in (%SHADER_DIR%\*.vert) do (
    glslc "%%f" -o "%%~dpnxf"
)
:: Compile shaders
for %%f in (%SHADER_DIR%\*.frag) do (
    glslc "%%f" -o "%%~dpnxf"
)
:: Compile shaders
for %%f in (%SHADER_DIR%\*.comp) do (
    glslc "%%f" -o "%%~dpnxf"
)

:: Move all shader files to output directory
for %%f in (shaders\compiled\*.vert) do (
    copy "%%f" "%OUTPUT_DIR%"
)
for %%f in (shaders\compiled\*.frag) do (
    copy "%%f" "%OUTPUT_DIR%"
)
for %%f in (shaders\compiled\*.comp) do (
    copy "%%f" "%OUTPUT_DIR%"
)

endlocal

pause