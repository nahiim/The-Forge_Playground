
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

:: Compile vertex shaders
for %%f in (%SHADER_DIR%\*.vert) do (
    glslc "%%f" -o "%%~dpnxf"
)

:: Compile fragment shaders
for %%f in (%SHADER_DIR%\*.frag) do (
    glslc "%%f" -o "%%~dpnxf"
)

:: Compile compute shaders
for %%f in (%SHADER_DIR%\*.comp) do (
    glslc "%%f" -o "%%~dpnxf"
)

endlocal

:: Move all shader files to output directory

move *.vert "%OUTPUT_DIR%"
move *.comp "%OUTPUT_DIR%"
move *.frag "%OUTPUT_DIR%"

pause