/**
 * @file UTexture.h
 * @brief 텍스처 크기와 픽셀 형식의 공통 인터페이스에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "CoreUObject/UObject.h"
#include "Engine/ETextureFormat.h"

/**
 * @brief 텍스처 크기와 픽셀 형식의 공통 인터페이스를 담당합니다.
 */
class UTexture : public UObject {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(UTexture, UObject);

public:
    /**
     * @brief UTexture의 초기 상태를 구성합니다.
     */
    UTexture();

    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    ~UTexture() override;

public:
    /**
     * @brief 이 타입의 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UTexture, EClassFlags::Abstract);

    /**
     * @brief 텍스처의 가로 픽셀 수를 반환합니다.
     * @return 텍스처의 가로 픽셀 수를 나타내는 정수 값입니다.
     */
    virtual Uint32 GetSizeX() const = 0;

    /**
     * @brief 텍스처의 세로 픽셀 수를 반환합니다.
     * @return 텍스처의 세로 픽셀 수를 나타내는 정수 값입니다.
     */
    virtual Uint32 GetSizeY() const = 0;

    /**
     * @brief 텍스처의 픽셀 또는 깊이 형식을 반환합니다.
     * @return 요청한 텍스처 형식 또는 사용 용도 값입니다.
     */
    virtual ETextureFormat GetFormat() const = 0;

    /**
     * @brief 텍스처를 sRGB 색 공간으로 해석하는지 반환합니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool IsSRGB() const = 0;

    /**
     * @brief 구현 텍스처가 사용할 수 있는 크기와 이미지 구성을 갖추었는지 검사합니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool IsValid() const = 0;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
};
