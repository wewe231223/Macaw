/**
 * @file FMaterialSerialization.h
 * @brief 네이티브 재질 파일의 버전과 필수 필드 검증을 공유합니다.
 */
#pragma once

#include "Core/Archive/FArchive.h"
#include "RenderCore/FMaterialData.h"
#include <filesystem>
#include <initializer_list>
#include <rapidjson/document.h>

/** @brief 로드 중 필수 필드가 빠지면 아카이브 오류를 설정합니다. */
bool RequireMaterialFields(FArchive& Archive, std::initializer_list<std::string_view> Names);
/** @brief 재질 종류와 파일 및 고정 레이아웃 버전을 확인합니다. */
bool LoadMaterialDocument(const std::filesystem::path& Path, const char* Kind, rapidjson::Document& Document);
/** @brief 새 재질 문서에 종류와 버전 정보를 기록합니다. */
void InitializeMaterialDocument(const char* Kind, rapidjson::Document& Document);
/** @brief GPU 주소를 제외한 고정 재질 계수를 직렬화합니다. */
void SerializeMaterialData(FArchive& Archive, FMaterialGPUData& Data);
