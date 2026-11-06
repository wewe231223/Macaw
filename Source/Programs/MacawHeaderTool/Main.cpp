/**
 * @file Main.cpp
 * @brief MacawHeaderTool의 소스 탐색·모듈 코드 생성·진단 출력을 실행합니다.
 */

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "CodeGenerator.h"

/**
 * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
 */
namespace {

    // 헤더 툴 자신과 C# 소스는 엔진 헤더가 아닙니다.

    /**
     * @brief 헤더 툴·관리 코드를 제외한 검사 대상 경로인지 판정합니다.
     * @param[in] Path 작업에 사용할 파일 또는 프로젝트 상대 에셋 경로입니다.
     * @return 헤더 툴·관리 코드 경로가 아닌 검사 대상이면 true입니다.
     */
    bool IsScanned(const std::string& Path) {
        return !Path.starts_with("Source/Programs/") && !Path.starts_with("Source/Managed/");
    }

    /**
     * @brief 현재 소스에서 지정 확장자의 파일 경로를 정렬해 수집합니다.
     * @param[in] Root 작업 공간 루트 경로입니다.
     * @param[in] Extension 점을 포함한 파일 확장자입니다.
     * @return 작업 공간 상대 경로로 정렬한 파일 목록입니다.
     */
    std::vector<std::string> FindFiles(const std::string& Root, const std::string& Extension) {
        std::vector<std::string> Files;
        const std::filesystem::path Base{Root};

        for (const auto& Entry : std::filesystem::recursive_directory_iterator{Base / "Source"}) {
            if (!Entry.is_regular_file() || Entry.path().extension() != Extension) {
                continue;
            }

            const std::string Path{std::filesystem::relative(Entry.path(), Base).generic_string()};

            if (IsScanned(Path)) {
                Files.push_back(Path);
            }
        }

        std::sort(Files.begin(), Files.end());

        return Files;
    }

    /**
     * @brief 모든 헤더를 파싱하고 요청 모듈의 리플렉션 코드·바인딩을 생성합니다.
     * @param[in] Workspace 원본 헤더와 루트 경로를 가진 작업 공간입니다.
     * @param[in] Module 생성할 모듈 이름이며 비어 있으면 전체입니다.
     */
    void Generate(HeaderTool::FWorkspace& Workspace, const std::string& Module) {
        for (const std::string& Path : FindFiles(Workspace.Root, ".h")) {
            Workspace.Headers.push_back(HeaderTool::ParseHeader(HeaderTool::ReadSourceFile(Workspace.Root, Path)));
        }

        for (const std::string& Path : FindFiles(Workspace.Root, ".cpp")) {
            HeaderTool::CheckSourceFile(HeaderTool::ReadSourceFile(Workspace.Root, Path));
        }

        std::vector<std::string> Modules;

        if (Module.empty()) {
            for (const auto& Entry : std::filesystem::directory_iterator{std::filesystem::path{Workspace.Root} / "Source"}) {
                const std::string Name{Entry.path().filename().string()};

                if (Entry.is_directory() && IsScanned("Source/" + Name + "/")) {
                    Modules.push_back(Name);
                }
            }
        } else {
            Modules.push_back(Module);
        }

        for (const std::string& Name : Modules) {
            HeaderTool::WriteGeneratedHeaders(Workspace, Name);

            if (HeaderTool::GetRegistrationModule(Name) == Name) {
                HeaderTool::WriteModuleReflection(Workspace, Name);
            }

            if (Name == "Script") {
                HeaderTool::WriteBindings(Workspace);
            }
        }
    }

}

/**
 * @brief 작업 공간의 리플렉션 코드와 C# 바인딩을 생성하고 오류를 빌드 진단으로 출력합니다.
 * @param[in] ArgumentCount 실행 파일 이름을 포함한 명령줄 인자 수입니다.
 * @param[in] Arguments 실행 파일·작업 공간·선택적 모듈 이름의 명령줄 배열입니다.
 * @return 생성 성공이면 0, 사용법 오류이면 2, 파싱·파일 오류이면 1입니다.
 */
int main(int ArgumentCount, char** Arguments) {
    if (ArgumentCount < 2 || ArgumentCount > 3) {
        std::cerr << "Usage: MacawHeaderTool <WorkspaceRoot> [Module]\n";

        return 2;
    }

    HeaderTool::FWorkspace Workspace;
    Workspace.Root = std::filesystem::absolute(std::filesystem::path{Arguments[1]}).lexically_normal().string();

    try {
        Generate(Workspace, ArgumentCount == 3 ? std::string{Arguments[2]} : std::string{});
    } catch (const HeaderTool::FHeaderToolError& Error) {
        std::filesystem::path File{std::filesystem::path{Workspace.Root} / std::filesystem::path{Error.Path}};

        std::cout << File.make_preferred().string();

        if (Error.Line > 0) {
            std::cout << "(" << Error.Line << ")";
        }

        std::cout << ": error HT0001: " << Error.Message << std::endl;

        return 1;
    } catch (const std::exception& Error) {
        std::cout << "MacawHeaderTool: error HT0002: " << Error.what() << std::endl;

        return 1;
    }

    return 0;
}
