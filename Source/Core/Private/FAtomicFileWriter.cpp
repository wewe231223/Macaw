/**
 * @file FAtomicFileWriter.cpp
 * @brief 임시 파일의 생성, 검증, 교체와 실패 시 정리를 구현합니다.
 * @ingroup Core
 */

#include "Core/FAtomicFileWriter.h"
#include "Core/FilePath.h"
#include "Core/Guid.h"

#include <Windows.h>

FAtomicFileWriter::~FAtomicFileWriter() {
    Discard();
}

bool FAtomicFileWriter::Open(const std::filesystem::path& Path, FString& Error) {
    Error.clear();
    Discard();
    if (Path.empty()) {
        Error = "A save path is required";
        return false;
    }

    std::error_code Code{};
    mTarget = std::filesystem::absolute(Path, Code);
    if (Code) {
        Error = "Invalid save path: " + Code.message();
        return false;
    }
    std::filesystem::create_directories(mTarget.parent_path(), Code);
    if (Code) {
        Error = "Cannot create save directory: " + Code.message();
        return false;
    }

    mTemporary = mTarget;
    mTemporary += FilePath::FromUtf8("." + FGuid{}.ToString() + ".tmp");
    mStream.clear();
    mStream.open(mTemporary, std::ios::binary | std::ios::trunc);
    if (!mStream.is_open()) {
        Error = "Cannot open temporary save file";
        Discard();
        return false;
    }
    return true;
}

std::ostream& FAtomicFileWriter::GetStream() {
    return mStream;
}

bool FAtomicFileWriter::Commit(FString& Error) {
    Error.clear();
    if (!mStream.is_open()) {
        Error = "No temporary save file is open";
        return false;
    }
    mStream.flush();
    mStream.close();
    if (!mStream) {
        Error = "Cannot write temporary save file";
        Discard();
        return false;
    }
    if (!MoveFileExW(mTemporary.c_str(), mTarget.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        Error = "Cannot replace save file";
        Discard();
        return false;
    }
    mTemporary.clear();
    mTarget.clear();
    return true;
}

void FAtomicFileWriter::Discard() {
    if (mStream.is_open()) {
        mStream.close();
    }
    if (!mTemporary.empty()) {
        std::error_code Code{};
        std::filesystem::remove(mTemporary, Code);
        mTemporary.clear();
    }
    mTarget.clear();
}
