vcpkg_from_github(
    OUT_SOURCE_PATH SourcePath
    REPO Organic-Code/ImTerm
    REF c720144e5b03b068257309f9e5391af26b903c9e
    SHA512 d3115cd10eb8ce430f2573d51cc1949bc4f207995e29a287b8d75ca6c9b8c471a3b2dc021555e7861d0b4b1c18dd0a68d44b1911f2f2895393b022780c16be3c
    PATCHES embedded-output.patch
)

file(INSTALL "${SourcePath}/include/imterm" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
vcpkg_install_copyright(FILE_LIST "${SourcePath}/LICENSE")
