/**
 * @file FMaterialChunkSignature.h
 * @brief 텍스처 바인딩 순서로 머티리얼 청크를 구별하는 서명과 빌더를 제공합니다.
 */

#pragma once
#include "Core/Base/FAssetHandle.h"

#include <array>
#include <functional>

/**
 * @brief 머티리얼 청크 서명에 저장할 수 있는 텍스처 필드 수의 상한입니다.
 */
inline constexpr Uint8 MaxMaterialTextureFields{12};

/**
 * @brief 텍스처 필드 수와 순서 있는 에셋 핸들을 보관하는 머티리얼 청크 서명입니다.
 * @note 텍스처 객체를 소유하지 않습니다. 동등성 비교는 사용하지 않는 배열 꼬리를 포함한 전체 저장 값을 비교합니다.
 */
struct FMaterialChunkSignature {
    /**
     * @brief 필드 순서의 텍스처 핸들 배열입니다. 사용 구간은 mTextureFieldCount로 결정합니다.
     */
    std::array<FAssetHandle, MaxMaterialTextureFields> mTextureHandles{};

    /**
     * @brief 사용 중인 텍스처 필드 수입니다. MaxMaterialTextureFields 이하여야 합니다.
     */
    Uint8 mTextureFieldCount{0};

    /**
     * @brief 텍스처 필드 수가 저장 배열의 용량 이하인지 검사합니다.
     * @return 필드 수가 허용 범위이면 true입니다.
     * @note 개별 핸들의 유효성이나 에셋 생존 여부는 검사하지 않습니다.
     */
    bool IsValid() const;

    /**
     * @brief 사용 구간의 텍스처 핸들을 조회합니다.
     * @param TextureFieldIndex 0부터 시작하는 텍스처 필드 인덱스입니다.
     * @pre 서명의 필드 수가 배열 용량 이하여야 합니다.
     * @return 해당 핸들입니다. 사용 구간 밖이면 무효 핸들입니다.
     */
    FAssetHandle GetTextureHandle(Uint8 TextureFieldIndex) const;

    /**
     * @brief 필드 수와 사용 구간의 핸들 ID·세대로 해시를 계산합니다.
     * @pre 서명의 필드 수가 배열 용량 이하여야 합니다.
     * @return 텍스처 순서를 반영한 해시입니다. 배열의 사용하지 않는 꼬리는 해시에 포함하지 않습니다.
     */
    std::size_t GetHash() const noexcept;

    /**
     * @brief 필드 수와 전체 핸들 배열을 정확히 비교합니다.
     * @param Other 비교할 서명입니다.
     * @return 모든 저장 값이 같으면 true입니다.
     */
    bool operator==(const FMaterialChunkSignature& Other) const = default;

    /**
     * @brief 필드 수 또는 전체 핸들 배열의 차이를 검사합니다.
     * @param Other 비교할 서명입니다.
     * @return 저장 값 중 하나라도 다르면 true입니다.
     */
    bool operator!=(const FMaterialChunkSignature& Other) const = default;
};

/**
 * @brief 고정 용량 안에서 텍스처 핸들을 순서대로 추가하는 서명 빌더입니다.
 */
class FMaterialChunkSignatureBuilder {
public:
    /**
     * @brief 다음 텍스처 필드에 핸들을 추가합니다.
     * @param TextureHandle 추가할 핸들입니다. 개별 핸들 유효성은 검사하지 않습니다.
     * @return 추가하면 true이고 용량이 가득 찼으면 false입니다. 실패 시 기존 서명을 유지합니다.
     */
    bool AddTexture(FAssetHandle TextureHandle);

    /**
     * @brief 현재까지 구성한 서명의 복사본을 반환합니다.
     * @return 현재 서명입니다. 빌더 상태는 유지합니다.
     */
    FMaterialChunkSignature Build() const;

    /**
     * @brief 필드 수와 전체 핸들 배열을 기본값으로 비웁니다.
     */
    void Reset();

private:
    /**
     * @brief 구성 중인 텍스처 필드 서명입니다.
     */
    FMaterialChunkSignature mSignature{};
};

/**
 * @brief 엔진 서명 타입에 대한 표준 해시 특수화를 선언하는 네임스페이스입니다.
 */
namespace std {
    /**
     * @brief FMaterialChunkSignature의 GetHash를 사용하는 표준 해시 특수화입니다.
     */
    template <>
    struct hash<FMaterialChunkSignature> {
        /**
         * @brief 머티리얼 청크 서명의 해시를 반환합니다.
         * @param Signature 필드 수가 배열 용량 이하인 서명입니다.
         * @return Signature.GetHash()의 결과입니다.
         */
        std::size_t operator()(const FMaterialChunkSignature& Signature) const noexcept;
    };
}
