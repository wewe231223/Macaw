vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SourcePath
    REPO Daandelange/im-neo-sequencer
    REF 4569c9bfa9cac87c5a9f2cc64b72b5de0cdcd6bd
    SHA512 1abd7ad175439ed19c99bc30ab54e5791f2dae18bc93e6a6d9510911137b49a4c99782e2540b8565abce4b63d9fb29f6bc3a4d21c4f09d02da8cf4fadc0b22ef
)

# Macaw의 키 드래그가 픽셀을 프레임으로 환산할 수 있도록 내부 프레임 폭 조회 함수를 공개합니다.
vcpkg_replace_string("${SourcePath}/imgui_neo_sequencer.h"
    "IMGUI_API NeoFrameRange NeoGetViewRange();"
    "IMGUI_API NeoFrameRange NeoGetViewRange();\n    IMGUI_API float NeoGetFrameWidth();"
)
vcpkg_replace_string("${SourcePath}/imgui_neo_sequencer.cpp"
    "    NeoFrameRange NeoGetViewRange()"
    "    float NeoGetFrameWidth()\n    {\n        IM_ASSERT(inSequencer && \"Not in active sequencer!\");\n        return getPerFrameWidth(sequencerData[currentSequencer]);\n    }\n\n    NeoFrameRange NeoGetViewRange()"
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SourcePath}")

vcpkg_cmake_configure(SOURCE_PATH "${SourcePath}")
vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SourcePath}/LICENSE")
