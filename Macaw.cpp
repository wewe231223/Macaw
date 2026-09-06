// Macaw.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#include "PCH.h"
 
#include "framework.h"
#include "Macaw.h"

#include "Render/Renderer.h"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
  
#include "Render/Renderer.h"
#include "Core/Console/Console.h"
#include "Render/Console/ConsoleWindow.h"
#include "Core/Asset/FAssetRegistry.h"

#include "Core/Base/TypeRegistry.h"

//test
#include "Render/Pipeline/UPipeline.h"
#include "Core/Asset/UMesh.h"
#include "Core/Asset/UColorMaterial.h"

#define MAX_LOADSTRING 100


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")

constexpr bool WINDOWED = true;
constexpr uint32 DEFAULT_WINDOW_WIDTH = 1920;
constexpr uint32 DEFAULT_WINDOW_HEIGHT = 1080;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

HWND hWnd = nullptr;

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HWND gHWND;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.
	TypeRegistry::Register(UObject::StaticTypeInfo());
    TypeRegistry::Register(UMesh::StaticTypeInfo());
    TypeRegistry::Register(UPipeline::StaticTypeInfo());


    auto res = TypeRegistry::Find("UMesh")->Creator();
	if (res->GetTypeInfo()->IsA(UMesh::StaticTypeInfo())) {
		Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "UMesh instance created successfully.");
	}
	else {
		Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to create UMesh instance.");
	}


    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MACAW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MACAW));

    MSG msg;

	Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Macaw Engine Initialized.");

	FRenderer Renderer;
	Renderer.Create(gHWND, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
	
    FAssetRegistry AssetRegistry;
	AssetRegistry.Initialize(Renderer.GetDevice(), 128);
	Renderer.BindAssetRegistry(&AssetRegistry);

	AssetRegistry.EmplaceAsset<UPipeline>(Renderer.GetDevice(), EAssetType::Pipeline, "BasePipeline", "./Pipeline/Base.json");

    std::vector<FVector3> Positions{
    { -0.5f, -0.5f, 0.0f },
    {  0.0f,  0.5f, 0.0f },
    {  0.5f, -0.5f, 0.0f }
    };

    std::vector<FVector3> Normals{
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f, -1.0f }
    };

    std::vector<FVector2D> UVs{
        { 0.0f, 1.0f },
        { 0.5f, 0.0f },
        { 1.0f, 1.0f }
    };

    TArray<uint32> Indices{
        0, 1, 2
    };

    
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), EAssetType::Mesh, "TriangleMesh",
        Indices, 
        MakeVertexAttribute<EVertexAttribute::Position>(Positions), 
        MakeVertexAttribute<EVertexAttribute::Normal>(Normals), 
        MakeVertexAttribute<EVertexAttribute::UV>(UVs)
    );

	auto handle = AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), EAssetType::Material, "BaseMaterial", FVector4{ 1.0f, 0.0f, 0.0f, 1.0f });

	AssetRegistry.ModifyAsset<UColorMaterial>(EAssetType::Material, handle, [](UColorMaterial& Material) {
		Material.SetColor(FVector4{ 0.0f, 1.0f, 0.0f, 1.0f });
	});

	AssetRegistry.GetMaterialBuffer().Flush(Renderer.GetDeviceContext());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            Renderer.BeginFrame();


            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            FRenderProbe RenderProbe;
            Renderer.Render(RenderProbe);

            DrawConsole(Console::STDOutHandle);

            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            Renderer.EndFrame();
        }
    }
   

    // ImGui 소멸
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();


    return (int) msg.wParam;
}


//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MACAW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.


    if (WINDOWED) {
        DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;
        DWORD exStyle = WS_EX_OVERLAPPEDWINDOW;

        int posX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_WIDTH) / 2);
        int posY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_HEIGHT) / 2);

        RECT adjustedRect{ 0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT };
        ::AdjustWindowRectEx(std::addressof(adjustedRect), style, FALSE, exStyle);

        hWnd = CreateWindowEx(
            exStyle,                                // 확장 스타일
            szWindowClass,                          // 윈도우 클래스 이름
            szTitle,                                // 윈도우 타이틀 
            style,                                  // 윈도우 스타일
            posX, posY,                             // 위치
            adjustedRect.right - adjustedRect.left,
            adjustedRect.bottom - adjustedRect.top, // 크기
            nullptr,                                // 부모 윈도우
            nullptr,                                // 메뉴
            hInstance,                              // 인스턴스 핸들
            nullptr                                 // 추가 매개변수
        );
    }
    else {
        DWORD style = WS_POPUP;
        DWORD exStyle = NULL;

        int posX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_WIDTH) / 2);
        int posY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_HEIGHT) / 2);

        hWnd = CreateWindowEx(
            exStyle,                                // 확장 스타일
            szWindowClass,                          // 윈도우 클래스 이름
            szTitle,                                // 윈도우 타이틀 
            style,                                  // 윈도우 스타일
            posX, posY,                             // 위치 
            DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, // 크기
            nullptr,                                // 부모 윈도우
            nullptr,                                // 메뉴
            hInstance,                              // 인스턴스 핸들
            nullptr                                 // 추가 매개변수
        );
    }

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

	gHWND = hWnd;

    return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);


    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


