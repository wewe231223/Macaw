/**
 * @file FProjectBrowser.cpp
 * @brief 프로젝트 열기·생성 및 최근 프로젝트 선택 UI를 처리합니다.
 * @ingroup Editor
 */

#include "Editor/FProjectBrowser.h"
#include "Core/FilePath.h"
#include "Editor/FProjectDialog.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "Core/Base/ErrorHandler.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <shlobj.h>
#include <algorithm>
#include <fstream>

/**
 * @brief Win32 입력 메시지를 ImGui 플랫폼 백엔드로 전달합니다.
 * @param[in] Window 입력 메시지의 대상 창입니다.
 * @param[in] Message Win32 메시지 번호입니다.
 * @param[in] WParam Win32 메시지의 첫 인수입니다.
 * @param[in] LParam Win32 메시지의 둘째 인수입니다.
 * @return 계산하거나 조회한 값입니다.
 */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

/** @brief 프로젝트 선택 UI와 최근 프로젝트 목록의 내부 보조 함수입니다. */
namespace {
    /**
     * @brief Windows의 알려진 폴더 식별자를 실제 경로로 해석합니다.
     * @param[in] Folder Windows의 알려진 폴더 식별자입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    std::filesystem::path KnownFolder(REFKNOWNFOLDERID Folder) {
        PWSTR Buffer{};
        if (FAILED(SHGetKnownFolderPath(Folder, 0, nullptr, &Buffer))) {
            return {};
        }
        const std::filesystem::path Path{Buffer};
        CoTaskMemFree(Buffer);
        return Path;
    }

    /**
     * @brief 최근 프로젝트 목록을 저장할 사용자 파일 경로를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    std::filesystem::path RecentProjectsFile() {
        const std::filesystem::path Directory{KnownFolder(FOLDERID_LocalAppData)};
        return Directory.empty() ? std::filesystem::path{} : Directory / "Macaw/RecentProjects.txt";
    }
}

FProjectBrowser::FProjectBrowser() {
    const FString Location{FilePath::ToUtf8(KnownFolder(FOLDERID_Documents) / "Macaw Projects")};
    std::copy_n(Location.data(), std::min(Location.size(), mLocation.size() - 1), mLocation.data());
    const FString Name{"NewProject"};
    std::copy(Name.begin(), Name.end(), mProjectName.begin());
}

bool FProjectBrowser::Show(FWindowsApplication& Platform, FRenderer& Renderer, std::filesystem::path& ProjectFile, bool& Create, const FString& Error) {
    mCreatePage = Create;
    mSubmitted = false;
    mSelectedFile = ProjectFile;
    if (Create && !ProjectFile.empty()) {
        const FString Name{FilePath::ToUtf8(ProjectFile.stem())};
        const FString Location{FilePath::ToUtf8(ProjectFile.parent_path().filename() == ProjectFile.stem() ? ProjectFile.parent_path().parent_path() : ProjectFile.parent_path())};
        mProjectName.fill(0);
        mLocation.fill(0);
        std::copy_n(Name.data(), std::min(Name.size(), mProjectName.size() - 1), mProjectName.data());
        std::copy_n(Location.data(), std::min(Location.size(), mLocation.size() - 1), mLocation.data());
    }
    LoadRecentProjects();
    SetWindowTextW(Platform.GetWindowHandle(), L"Macaw - Project Browser");
    if (!Renderer.InitializeUi(Platform.GetWindowHandle(), Platform.GetWidth(), Platform.GetHeight())) {
        ErrorHandler::Fatal("ProjectBrowser", "Cannot initialize the project browser renderer");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& Input{ImGui::GetIO()};
    Input.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    Input.IniFilename = nullptr;
    Input.LogFilename = nullptr;
    if (std::filesystem::exists("C:/Windows/Fonts/malgun.ttf")) {
        Input.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 18.0f);
    } else {
        Input.Fonts->AddFontDefault();
    }
    ImGui::StyleColorsDark();
    ImGuiStyle& Style{ImGui::GetStyle()};
    Style.WindowPadding = ImVec2{32.0f, 28.0f};
    Style.FramePadding = ImVec2{12.0f, 10.0f};
    Style.ItemSpacing = ImVec2{12.0f, 14.0f};
    Style.FrameRounding = 5.0f;
    Style.ChildRounding = 8.0f;
    Style.Colors[ImGuiCol_WindowBg] = ImVec4{0.045f, 0.055f, 0.075f, 1.0f};
    Style.Colors[ImGuiCol_ChildBg] = ImVec4{0.075f, 0.09f, 0.12f, 1.0f};
    Style.Colors[ImGuiCol_Button] = ImVec4{0.11f, 0.31f, 0.48f, 1.0f};
    Style.Colors[ImGuiCol_ButtonHovered] = ImVec4{0.15f, 0.43f, 0.63f, 1.0f};
    Style.Colors[ImGuiCol_ButtonActive] = ImVec4{0.10f, 0.36f, 0.55f, 1.0f};

    const bool Win32Ready{ImGui_ImplWin32_Init(Platform.GetWindowHandle())};
    const bool DxReady{Win32Ready && ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(Renderer.GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext()))};
    if (DxReady) {
        Platform.SetMessageHandler([](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
            return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
        });
        while (!mSubmitted && Platform.PumpMessages()) {
            if (Platform.IsMinimized()) {
                MsgWaitForMultipleObjects(0, nullptr, false, 16, QS_ALLINPUT);
                continue;
            }
            Renderer.BeginFrame(Platform.GetWidth(), Platform.GetHeight());
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            Draw(Platform, Error);
            ImGui::Render();
            Renderer.BeginUiRender();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            Renderer.Present();
        }
    }
    Platform.SetMessageHandler({});
    if (DxReady) {
        ImGui_ImplDX11_Shutdown();
    }
    if (Win32Ready) {
        ImGui_ImplWin32_Shutdown();
    }
    ImGui::DestroyContext();
    Renderer.Shutdown();
    if (!DxReady) {
        ErrorHandler::Fatal("ProjectBrowser", "Cannot initialize the project browser UI");
    }
    if (mSubmitted) {
        Create = mCreatePage;
        ProjectFile = mSelectedFile;
    }
    return mSubmitted;
}

void FProjectBrowser::Draw(FWindowsApplication& Platform, const FString& Error) {
    const ImGuiViewport* Viewport{ImGui::GetMainViewport()};
    ImGui::SetNextWindowPos(Viewport->WorkPos);
    ImGui::SetNextWindowSize(Viewport->WorkSize);
    ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PushFont(nullptr, 36.0f);
    ImGui::TextUnformatted("MACAW");
    ImGui::PopFont();
    ImGui::TextDisabled("Choose a project or start something new.");
    ImGui::Spacing();
    if (ImGui::Button("Open Project", ImVec2{180.0f, 44.0f})) {
        mCreatePage = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("New Project", ImVec2{180.0f, 44.0f})) {
        mCreatePage = true;
    }
    ImGui::Separator();
    const float Width{std::min(960.0f, ImGui::GetContentRegionAvail().x)};
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - Width) * 0.5f);
    ImGui::BeginChild("Project Content", ImVec2{Width, 0.0f});
    if (!Error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.48f, 0.42f, 1.0f});
        ImGui::TextWrapped("%s", Error.c_str());
        ImGui::PopStyleColor();
    }
    if (mCreatePage) {
        DrawNewProject(Platform);
    } else {
        DrawOpenProject(Platform);
    }
    ImGui::EndChild();
    ImGui::End();
}

void FProjectBrowser::DrawOpenProject(FWindowsApplication& Platform) {
    ImGui::TextUnformatted("Recent projects");
    ImGui::BeginChild("Recent Projects", ImVec2{0.0f, std::max(120.0f, ImGui::GetContentRegionAvail().y - 160.0f)}, ImGuiChildFlags_Borders);
    if (mRecentProjects.empty()) {
        ImGui::TextDisabled("No projects opened yet.");
        ImGui::TextWrapped("Use Browse to select an existing .mproject file, or create a new project.");
    }
    for (std::size_t Index{}; Index < mRecentProjects.size(); ++Index) {
        const auto& Path{mRecentProjects[Index]};
        const FString Label{FilePath::ToUtf8(Path.stem()) + "##" + std::to_string(Index)};
        if (ImGui::Selectable(Label.c_str(), mSelectedFile == Path, ImGuiSelectableFlags_AllowDoubleClick)) {
            mSelectedFile = Path;
            mSubmitted = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        }
        ImGui::TextDisabled("%s", FilePath::ToUtf8(Path).c_str());
        ImGui::Separator();
    }
    ImGui::EndChild();
    if (ImGui::Button("Browse...", ImVec2{140.0f, 0.0f})) {
        const auto Path{FProjectDialog::Choose(Platform.GetWindowHandle(), false)};
        if (!Path.empty()) {
            mSelectedFile = Path;
        }
    }
    ImGui::SameLine();
    ImGui::TextWrapped("%s", mSelectedFile.empty() ? "Select a .mproject file" : FilePath::ToUtf8(mSelectedFile).c_str());
    ImGui::BeginDisabled(mSelectedFile.empty());
    if (ImGui::Button("Open Selected Project", ImVec2{240.0f, 44.0f})) {
        mSubmitted = true;
    }
    ImGui::EndDisabled();
}

void FProjectBrowser::DrawNewProject(FWindowsApplication& Platform) {
    ImGui::TextUnformatted("Create a new project");
    ImGui::BeginChild("Default Template", ImVec2{0.0f, 140.0f}, ImGuiChildFlags_Borders);
    ImGui::PushFont(nullptr, 24.0f);
    ImGui::TextUnformatted("Default");
    ImGui::PopFont();
    ImGui::TextWrapped("A minimal starting scene with a cube and a camera.");
    ImGui::TextWrapped("Includes project shaders and a Content folder for your assets.");
    ImGui::EndChild();
    ImGui::TextUnformatted("Project name");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##ProjectName", mProjectName.data(), mProjectName.size());
    ImGui::TextUnformatted("Location");
    ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x - 150.0f));
    ImGui::InputText("##Location", mLocation.data(), mLocation.size());
    ImGui::SameLine();
    if (ImGui::Button("Browse...", ImVec2{138.0f, 0.0f})) {
        const auto Path{FProjectDialog::ChooseDirectory(Platform.GetWindowHandle())};
        if (!Path.empty()) {
            const FString Location{FilePath::ToUtf8(Path)};
            mLocation.fill(0);
            std::copy_n(Location.data(), std::min(Location.size(), mLocation.size() - 1), mLocation.data());
        }
    }
    const bool ValidName{IsProjectNameValid()};
    const bool ValidLocation{mLocation[0] != '\0' && FilePath::FromUtf8(mLocation.data()).is_absolute()};
    if (ValidName && ValidLocation) {
        ImGui::TextDisabled("Project folder");
        ImGui::TextWrapped("%s", FilePath::ToUtf8(GetNewProjectFile().parent_path()).c_str());
    } else {
        ImGui::TextWrapped("Enter a valid project name and an absolute location.");
    }
    ImGui::BeginDisabled(!ValidName || !ValidLocation);
    if (ImGui::Button("Create Project", ImVec2{220.0f, 44.0f})) {
        mSelectedFile = GetNewProjectFile();
        mSubmitted = true;
    }
    ImGui::EndDisabled();
}

std::filesystem::path FProjectBrowser::GetNewProjectFile() const {
    const std::filesystem::path Name{FilePath::FromUtf8(mProjectName.data())};
    return FilePath::FromUtf8(mLocation.data()) / Name / (Name.wstring() + L".mproject");
}

bool FProjectBrowser::IsProjectNameValid() const {
    const FString Name{mProjectName.data()};
    if (Name.empty() || Name == "." || Name == ".." || Name.front() == ' ' || Name.back() == ' ' || Name.back() == '.') {
        return false;
    }
    for (const unsigned char Character : Name) {
        if (Character < 32 || FString{"<>:\"/\\|?*"}.find(static_cast<char>(Character)) != FString::npos) {
            return false;
        }
    }
    return true;
}

void FProjectBrowser::LoadRecentProjects() {
    mRecentProjects.clear();
    std::ifstream Input{RecentProjectsFile(), std::ios::binary};
    FString Line{};
    while (mRecentProjects.size() < 12 && std::getline(Input, Line)) {
        std::error_code Code{};
        const std::filesystem::path Path{FilePath::FromUtf8(Line.c_str())};
        if (Path.extension() == L".mproject" && std::filesystem::is_regular_file(Path, Code) && std::find(mRecentProjects.begin(), mRecentProjects.end(), Path) == mRecentProjects.end()) {
            mRecentProjects.push_back(Path);
        }
    }
}

void FProjectBrowser::RememberProject(const std::filesystem::path& ProjectFile) {
    FProjectBrowser Browser{};
    Browser.LoadRecentProjects();
    std::erase(Browser.mRecentProjects, ProjectFile);
    Browser.mRecentProjects.insert(Browser.mRecentProjects.begin(), ProjectFile);
    const std::filesystem::path File{RecentProjectsFile()};
    std::error_code Code{};
    if (File.empty()) {
        return;
    }
    std::filesystem::create_directories(File.parent_path(), Code);
    if (Code) {
        return;
    }
    std::ofstream Output{File, std::ios::binary | std::ios::trunc};
    for (std::size_t Index{}; Index < std::min<std::size_t>(12, Browser.mRecentProjects.size()); ++Index) {
        Output << FilePath::ToUtf8(Browser.mRecentProjects[Index]) << '\n';
    }
}
