@echo off
setlocal
set "MeshMaterialLibDirectory=bin\Debug\x64"
if not defined MeshMaterialLibDirectory set "MeshMaterialLibDirectory=bin\Debug\x64"
cd /d "%~dp0.."
if not exist Build\Diagnostics mkdir Build\Diagnostics
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /Zc:preprocessor /EHsc /MDd /Zi /Od /utf-8 /D_DEBUG /DUNICODE /D_UNICODE /DNOMINMAX /ICore\Public /ICoreUObject\Public /IWorld\Public /IEngine\Public /IAsset\Public /IRenderCore\Public /ISerialization\Public /IEditor\Public /IRender\Public /IImGui\Public /IPlatform\Public /IExternals\Include /Ivcpkg_installed\x64-windows\include Scripts\Regression\MeshMaterialRegression.cpp /FoBuild\Diagnostics\MeshMaterialRegression.obj /FeBuild\Diagnostics\MeshMaterialRegression.exe /FdBuild\Diagnostics\MeshMaterialRegression.pdb /link /LIBPATH:"%MeshMaterialLibDirectory%" /LIBPATH:Externals\bin\debug /LIBPATH:vcpkg_installed\x64-windows\debug\lib Editor.lib Engine.lib World.lib Asset.lib CoreUObject.lib Core.lib RenderCore.lib Serialization.lib Render.lib ImGui.lib Platform.lib DirectXTex.lib freetyped.lib d3d11.lib dxgi.lib d3dcompiler.lib dwmapi.lib gdi32.lib imm32.lib shell32.lib user32.lib kernel32.lib ole32.lib comdlg32.lib /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
Build\Diagnostics\MeshMaterialRegression.exe %*
exit /b %errorlevel%
