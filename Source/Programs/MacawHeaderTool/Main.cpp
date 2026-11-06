#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "CodeGenerator.h"

namespace {

    // 헤더 툴 자신과 C# 소스는 엔진 헤더가 아닙니다.
    bool IsScanned(const std::string& Path) {
        return !Path.starts_with("Source/Programs/") && !Path.starts_with("Source/Managed/");
    }

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
