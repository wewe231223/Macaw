/**
 * @file FEditorConsoleCommands.cpp
 * @brief 에디터·렌더링·통계 설정을 콘솔로 조회하고 변경합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorContext.h"
#include "Core/FilePath.h"
#include "Core/Stats.h"
#include "Editor/FEditorViewport.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/IRendererModule.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <sstream>
#include <type_traits>
#include <utility>

namespace Editor {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 문자열을 소문자로 변환해 반환합니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 해석·변환한 문자열입니다.
         */
        Core::FString Lowercase(std::string_view Text) {
            Core::FString Lower{Text};
            std::transform(Lower.begin(), Lower.end(), Lower.begin(), [](unsigned char Character) {
                return static_cast<char>(std::tolower(Character));
            });

            return Lower;
        }

        /**
         * @brief 콘솔 인자에서 공백을 정리한 단일 값을 읽고 잘못된 입력을 거부합니다.
         * @param[in] Arguments 호출·파싱에 사용할 인자 목록입니다.
         * @return 유효한 단일 인자이며 잘못된 입력이면 빈 optional입니다.
         */
        std::optional<Core::FString> ReadConsoleValue(std::string_view Arguments) {
            std::istringstream Input{Core::FString{Arguments}};
            Core::FString Value{};
            Core::FString Extra{};

            Input >> Value >> Extra;

            if (!Extra.empty()) {
                return std::nullopt;
            }

            return Lowercase(Value);
        }

        /**
         * @brief 콘솔 호출 정보에서 사용법 안내 문자열을 구성합니다.
         * @param[in] Invocation 명령 이름·사용법·입력·인자입니다.
         * @return 해석·변환한 문자열입니다.
         */
        Core::FString MakeUsage(const Editor::FConsoleInvocation& Invocation) {
            Core::FString Usage{"Usage: "};

            Usage += Invocation.mName;

            if (!Invocation.mUsage.empty()) {
                Usage += ' ';
                Usage += Invocation.mUsage;
            }

            return Usage;
        }
    }

    /**
     * @brief 편집 기능·렌더 설정·통계 명령을 콘솔에 등록합니다.
     */
    void Editor::FEditorContext::RegisterConsoleCommands() {
        mConsoleCommands.Register({"clear", {}, {}, [this](const Editor::FConsoleInvocation&) {
            ClearLogs();
        }});
        mConsoleCommands.Register({"help", {}, {}, [this](const Editor::FConsoleInvocation&) {
            AddLog("Available commands (Up/Down for history, Ctrl+Up/Ctrl+Down to select a suggestion, Tab to complete)");

            for (const Core::FString& Line : mConsoleCommands.GetHelp()) {
                AddLog(Line);
            }
        }});

        const auto SceneCommand{[this](bool (Editor::FEditorContext::*Operation)(const std::filesystem::path&)) {
            return [this, Operation](const Editor::FConsoleInvocation& Invocation) {
                const std::filesystem::path Path{Invocation.mArguments.empty() ? mScenePath : Core::FilePath::FromUtf8(Core::FString{Invocation.mArguments})};

                if (Path.empty()) {
                    AddLog("Specify a scene path.");
                } else {
                    (this->*Operation)(Path);
                }
            };
        }};
        mConsoleCommands.Register({"load", "[Maps/Scene.masset]", {}, SceneCommand(&Editor::FEditorContext::LoadScene)});
        mConsoleCommands.Register({"save", "[Maps/Scene.masset]", {}, SceneCommand(&Editor::FEditorContext::SaveScene)});
        mConsoleCommands.Register({"spawn", "<plane/cube/sphere>", {"plane", "cube", "sphere"}, [this](const Editor::FConsoleInvocation& Invocation) {
            std::istringstream Input{Core::FString{Invocation.mArguments}};
            Core::FString Type{};
            Input >> Type;

            if (Type == "plane" || Type == "cube" || Type == "sphere") {
                SpawnPrimitive(Type == "plane" ? Engine::EPrimitiveType::Plane : Type == "sphere" ? Engine::EPrimitiveType::Sphere : Engine::EPrimitiveType::Cube);
            } else {
                AddLog("Expected plane, cube or sphere.");
            }
        }});

        mConsoleCommands.Register({"Allow Tearing", "[on/off/status]", {"on", "off", "status"}, [this](const Editor::FConsoleInvocation& Invocation) {
            ExecuteAllowTearing(Invocation);
        }});
        mConsoleCommands.Register({"Shadow Atlas", {}, {}, [this](const Editor::FConsoleInvocation& Invocation) {
            if (Invocation.mArguments.empty()) {
                SetShadowAtlasOpen(true);
            } else {
                AddLog(MakeUsage(Invocation));
            }
        }});

        const auto RegisterShadow{[this](Core::FString Name, Core::FString Usage, Core::TArray<Core::FString> Choices, auto Update, Core::FString Alias = {}) {
            Editor::FConsoleCommand Command{std::move(Name), std::move(Usage), std::move(Choices), [this, Update](const Editor::FConsoleInvocation& Invocation) {
                const std::optional<Core::FString> Value{ReadConsoleValue(Invocation.mArguments)};

                if (!Value) {
                    AddLog("Expected at most one value.");

                    return;
                }

                const Core::FString LogName{Lowercase(Invocation.mName)};
                RenderCore::IRendererModule* Renderer{GetRenderer()};

                if (Renderer == nullptr) {
                    AddLog(LogName + ": renderer is unavailable.");

                    return;
                }

                RenderCore::FShadowSettings Settings{Renderer->GetShadowSettings()};
                const std::optional<Core::FString> Current{Update(Settings, *Value)};

                if (!Current) {
                    AddLog(MakeUsage(Invocation));

                    return;
                }

                Renderer->SetShadowSettings(Settings);
                AddLog(LogName + " = " + *Current);
            }};

            if (!Alias.empty()) {
                mConsoleCommands.Register(Command);

                Command.mUsage += " (" + Command.mName + " alias)";
                Command.mName = std::move(Alias);
            }

            mConsoleCommands.Register(std::move(Command));
        }};
        const auto RegisterShadowNumber{[&RegisterShadow](Core::FString Name, Core::FString Usage, Core::TArray<Core::FString> Choices, auto Field, Core::Uint32 Minimum, Core::Uint32 Maximum, Core::Uint32 Step = 1, Core::FString Alias = {}) {
            RegisterShadow(std::move(Name), std::move(Usage), std::move(Choices), [Field, Minimum, Maximum, Step](RenderCore::FShadowSettings& Settings, std::string_view Value) -> std::optional<Core::FString> {
                auto& Current{Settings.*Field};

                if (!Value.empty()) {
                    Core::Uint32 Parsed{};
                    const auto [End, Error]{std::from_chars(Value.data(), Value.data() + Value.size(), Parsed)};

                    if (Error != std::errc{} || End != Value.data() + Value.size() || Parsed < Minimum || Parsed > Maximum || (Parsed - Minimum) % Step != 0) {
                        return std::nullopt;
                    }

                    Current = static_cast<std::remove_reference_t<decltype(Current)>>(Parsed);
                }

                return std::to_string(static_cast<Core::Uint32>(Current));
            }, std::move(Alias));
        }};
        RegisterShadowNumber("r.shadow.CacheWholeSceneShadows", "[0/1]", {"0", "1"}, &RenderCore::FShadowSettings::mCacheWholeSceneShadows, 0, 1, 1, "r.shadow.CWSS");
        RegisterShadowNumber("r.shadow.CSM.Count", "[0-4] (0 uses the light's CascadeCount)", {"0", "1", "2", "3", "4"}, &RenderCore::FShadowSettings::mCascadeOverride, 0, 4);
        RegisterShadowNumber("r.shadow.PCF.Size", "[3/5/7]", {"3", "5", "7"}, &RenderCore::FShadowSettings::mPCFSize, 3, 7, 2);
        RegisterShadowNumber("r.shadow.PCSS.MaxFilter", "[1-64]", {}, &RenderCore::FShadowSettings::mPCSSMaxFilterTexels, 1, 64);
        RegisterShadowNumber("r.shadow.SkipUnseenLights", "[0/1]", {"0", "1"}, &RenderCore::FShadowSettings::mSkipUnseenLights, 0, 1);
        RegisterShadow("r.shadow.Filter", "[hard/pcf/pcss]", {"hard", "pcf", "pcss"}, [](RenderCore::FShadowSettings& Settings, std::string_view Value) -> std::optional<Core::FString> {
            if (Value == "hard" || Value == "0") {
                Settings.mFilter = RenderCore::EShadowFilter::Hard;
            } else if (Value == "pcf" || Value == "1") {
                Settings.mFilter = RenderCore::EShadowFilter::PCF;
            } else if (Value == "pcss" || Value == "2") {
                Settings.mFilter = RenderCore::EShadowFilter::PCSS;
            } else if (!Value.empty()) {
                return std::nullopt;
            }

            return Settings.mFilter == RenderCore::EShadowFilter::Hard ? "hard" : Settings.mFilter == RenderCore::EShadowFilter::PCF ? "pcf" : "pcss";
        });

        const auto RegisterShowFlag{[this](Core::FString Name, bool Engine::FEngineShowFlags::*Field) {
            mConsoleCommands.Register({std::move(Name), "[on/off]", {"on", "off"}, [this, Field](const Editor::FConsoleInvocation& Invocation) {
                const std::optional<Core::FString> State{ReadConsoleValue(Invocation.mArguments)};

                if (!State || (!State->empty() && *State != "on" && *State != "off" && *State != "0" && *State != "1")) {
                    AddLog(MakeUsage(Invocation));
                    return;
                }

                Editor::FEditorViewport* Viewport{mConsoleViewportProvider ? mConsoleViewportProvider() : nullptr};

                if (Viewport == nullptr) {
                    AddLog("Viewport is unavailable.");
                    return;
                }

                Editor::FEditorViewportClient* Client{Viewport->GetClient()};
                Engine::FEngineShowFlags Flags{Client->GetShowFlags()};
                bool& Enabled{Flags.*Field};
                Enabled = State->empty() ? !Enabled : *State == "on" || *State == "1";
                Client->SetShowFlags(Flags);
                AddLog(Lowercase(Invocation.mName) + (Enabled ? ": on" : ": off"));
            }});
        }};
        RegisterShowFlag("ShowFlag.Fog", &Engine::FEngineShowFlags::mFog);
        RegisterShowFlag("ShowFlag.FXAA", &Engine::FEngineShowFlags::mFXAA);
        RegisterShowFlag("ShowFlag.Shadows", &Engine::FEngineShowFlags::mShadows);
        RegisterShowFlag("ShowFlag.Sky", &Engine::FEngineShowFlags::mSky);

        const auto ExecuteStat{[this](const Editor::FConsoleInvocation& Invocation) {
            Editor::FEditorViewport* Viewport{mConsoleViewportProvider ? mConsoleViewportProvider() : nullptr};

            if (Viewport == nullptr) {
                AddLog("Viewport is unavailable.");

                return;
            }

            Core::FString Output{};

            if (Viewport->GetStatistics()->ExecuteCommand(Core::FString{Invocation.mInput}, Output) && !Output.empty()) {
                AddLog(Output);
            }
        }};
        mConsoleCommands.Register({"stat", "[<group> [on/off]]", {}, ExecuteStat});
        mConsoleCommands.Register({"stat all", "[on/off]", {"on", "off"}, ExecuteStat});
        mConsoleCommands.Register({"stat list", {}, {}, ExecuteStat});
        mConsoleCommands.Register({"stat none", {}, {}, ExecuteStat});
        mConsoleCommands.Register({"stat reset", {}, {}, ExecuteStat});
        const auto ExecuteGraph{[this](const Editor::FConsoleInvocation& Invocation) {
            AddLog(mStatGraphs.ExecuteCommand(Invocation.mArguments));
        }};
        mConsoleCommands.RegisterSource([ExecuteStat, ExecuteGraph]() {
            Core::TArray<Editor::FConsoleCommand> Commands{};
            Core::TArray<Core::FString> GraphArguments{"all", "clear", "off"};

            for (const Core::FString& Group : Core::FStats::Get().GetGroups()) {
                const Core::FString Name{Lowercase(Group)};

                if (Name == "all" || Name == "list" || Name == "none" || Name == "reset" || Name == "shadows") {
                    continue;
                }

                Commands.push_back({"stat " + Group, "[on/off]", {"on", "off"}, ExecuteStat});
                GraphArguments.push_back(Group);

                if (Name == "shadow") {
                    Commands.push_back({"stat shadows", "[on/off] (stat shadow alias)", {"on", "off"}, ExecuteStat});
                }
            }

            Commands.push_back({"stat graph", "[<group> [<stat name>] | all | clear | off]", std::move(GraphArguments), ExecuteGraph});

            return Commands;
        });
    }

    /**
     * @brief 편집 컨텍스트가 소유하는 통계 그래프 창을 반환합니다.
     */
    Editor::FStatGraphs& Editor::FEditorContext::GetStatGraphs() {
        return mStatGraphs;
    }

    /**
     * @brief 콘솔 문자열을 공통 편집 동작으로 전달합니다.
     */
    void Editor::FEditorContext::ExecuteConsoleCommand(const Core::FString& Command) {
        AddLog("> " + Command);

        if (!mConsoleCommands.Execute(Command) && Command.find_first_not_of(" \t\n\r\f\v") != Core::FString::npos) {
            AddLog("Unknown command. Type help for the list.");
        }
    }

    /**
     * @brief 콘솔 명령에서 입력 접두어에 맞는 후보를 조회합니다.
     */
    Core::TArray<Core::FString> Editor::FEditorContext::GetConsoleSuggestions(std::string_view Input) const {
        return mConsoleCommands.GetSuggestions(Input);
    }

    /**
     * @brief 콘솔 명령이 사용할 뷰포트 제공자를 설정합니다.
     */
    void Editor::FEditorContext::SetConsoleViewportProvider(std::function<Editor::FEditorViewport*()> Provider) {
        mConsoleViewportProvider = std::move(Provider);
    }

    /**
     * @brief 콘솔 인자로 화면 찢김 허용 설정을 조회하거나 변경합니다.
     */
    void Editor::FEditorContext::ExecuteAllowTearing(const Editor::FConsoleInvocation& Invocation) {
        const std::optional<Core::FString> State{ReadConsoleValue(Invocation.mArguments)};

        if (!State || (!State->empty() && *State != "on" && *State != "off" && *State != "status")) {
            AddLog(MakeUsage(Invocation));

            return;
        }

        RenderCore::IRendererModule* Renderer{GetRenderer()};

        if (Renderer == nullptr) {
            AddLog("Allow Tearing: renderer is unavailable.");

            return;
        }

        if (*State == "status") {
            AddLog(Renderer->IsTearingSupported() ? Renderer->IsTearingAllowed() ? "Allow Tearing: on (VSync off in windowed mode)." : "Allow Tearing: off (VSync on)." : "Allow Tearing: unsupported (VSync on).");

            return;
        }

        if (!Renderer->SetAllowTearing(*State != "off")) {
            AddLog("Allow Tearing: unsupported or renderer is not initialized.");

            return;
        }

        AddLog(Renderer->IsTearingAllowed() ? "Allow Tearing: on (VSync off in windowed mode)." : "Allow Tearing: off (VSync on).");
    }

}
