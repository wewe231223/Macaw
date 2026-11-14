/**
 * @file UParticleSystem.h
 * @brief UObject 기반 파티클 시스템의 Emitter 소유 관계와 정의 직렬화를 선언합니다.
 * @ingroup Cascade
 */

/**
 * @defgroup Cascade 파티클 시스템
 * @brief UObject 기반 정의와 Context 기반 모듈 호출 및 인스턴스 페이로드를 제공합니다.
 * @details Core와 CoreUObject에 의존하며 에셋 파일 입출력과 렌더링은 외부 계층에서 담당합니다.
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/UObject.h"

#include <cstddef>
#include <memory>

#include "UParticleSystem.generated.h"

namespace Core {
    class FArchive;
}

/**
 * @brief 파티클 시스템 정의와 모듈 실행 컨텍스트 및 인스턴스 저장소를 제공합니다.
 */
namespace Cascade {

    class UParticleEmitter;

    /**
     * @brief Emitter 정의들을 소유하며 파티클 시스템 구성을 직렬화하는 UObject입니다.
     * @details 재생 중인 파티클이나 실행 페이로드는 소유하지 않습니다. Emitter의 Outer는 이 시스템으로 연결합니다.
     * @ingroup Cascade
     */
    UCLASS()
    class UParticleSystem : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief UObject 이름을 ParticleSystem으로 설정하고 빈 Emitter 목록을 준비합니다.
         */
        UParticleSystem();
        /**
         * @brief 자식 Emitter의 Outer 연결과 시스템이 가진 소유 참조를 해제합니다.
         */
        ~UParticleSystem() override;

    public:
        /**
         * @brief Emitter를 시스템에 추가하고 Outer를 이 시스템으로 연결합니다.
         * @param[in] Emitter 추가할 Emitter의 공유 소유 참조입니다.
         * @return 추가하면 true이며 null·중복·다른 Outer·개수 제한 등으로 거부하면 false입니다.
         */
        bool AddEmitter(const std::shared_ptr<Cascade::UParticleEmitter>& Emitter);
        /**
         * @brief 지정한 Emitter의 Outer 연결과 목록 소유 참조를 제거합니다.
         * @param[in] Emitter 제거할 Emitter의 비소유 주소입니다.
         * @return 목록에서 제거했으면 true이며 대상이 없으면 false입니다.
         */
        bool RemoveEmitter(const Cascade::UParticleEmitter* Emitter);
        /**
         * @brief 모든 자식 Emitter의 Outer 연결을 끊고 시스템의 소유 목록을 비웁니다.
         * @note 다른 공유 참조가 있는 Emitter의 객체 수명은 해당 참조가 결정합니다.
         */
        void ClearEmitters();
        /**
         * @brief UObject 자체를 이동하지 않고 다른 시스템의 Emitter 목록만 가져옵니다.
         * @param[in,out] Source Emitter 목록을 이전할 원본 시스템입니다. 자기 자신이면 아무 작업도 하지 않습니다.
         * @details 기존 목록을 비운 뒤 가져온 Emitter들의 Outer를 이 시스템으로 다시 연결합니다.
         */
        void MoveEmittersFrom(Cascade::UParticleSystem& Source);

        /**
         * @brief 인덱스에 해당하는 Emitter의 수정 가능한 비소유 주소를 반환합니다.
         * @param[in] Index Emitter 목록의 0부터 시작하는 인덱스입니다.
         * @return 대상 Emitter의 주소이며 범위를 벗어나면 nullptr입니다.
         */
        Cascade::UParticleEmitter* GetEmitter(Core::Index_t Index);
        /**
         * @brief 인덱스에 해당하는 Emitter의 읽기 전용 비소유 주소를 반환합니다.
         * @param[in] Index Emitter 목록의 0부터 시작하는 인덱스입니다.
         * @return 대상 Emitter의 주소이며 범위를 벗어나면 nullptr입니다.
         */
        const Cascade::UParticleEmitter* GetEmitter(Core::Index_t Index) const;
        /**
         * @brief 시스템이 보관하는 Emitter 정의의 개수를 반환합니다.
         * @return 현재 Emitter 목록의 원소 수입니다.
         */
        std::size_t GetEmitterCount() const;
        /**
         * @brief Emitter 정의를 공유 소유하는 목록을 읽기 전용으로 제공합니다.
         * @return 시스템이 보관하는 목록의 참조입니다.
         */
        const Core::TArray<std::shared_ptr<Cascade::UParticleEmitter>>& GetEmitters() const;

        /**
         * @brief 시스템 형식 정보와 Emitter 클래스·설정 목록을 아카이브로 읽거나 기록합니다.
         * @param[in,out] Archive 로드 또는 저장 모드의 아카이브입니다.
         * @details 로드는 등록된 UClass 팩토리로 Emitter를 만들고 전체 목록을 읽은 뒤 교체합니다. 실행 페이로드는 저장하지 않습니다.
         */
        void Serialize(Core::FArchive& Archive) override;

    private:
        /**
         * @brief 시스템의 자식 Emitter 정의를 공유 소유하는 목록입니다.
         */
        Core::TArray<std::shared_ptr<Cascade::UParticleEmitter>> mEmitters{};
    };

}
