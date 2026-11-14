/**
 * @file UParticleSystem.cpp
 * @brief 파티클 시스템의 Emitter 소유 관계와 리플렉션 기반 정의 직렬화를 구현합니다.
 * @ingroup Cascade
 */

#include "Core/Types.h"
#include "Cascade/UParticleSystem.h"
#include "Cascade/UParticleEmitter.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Archive.h"

#include <algorithm>
#include <utility>

using namespace Cascade;
using namespace Core;
using namespace CoreUObject;

/**
 * @brief 이 소스 파일 안에서만 사용하는 직렬화와 메모리 배치 보조 처리를 묶습니다.
 */
namespace {

    /**
     * @brief 파티클 시스템 JSON 본문에 기록하는 현재 형식 버전입니다.
     */
    constexpr Uint32 ParticleSystemFormatVersion{1};
    /**
     * @brief 시스템 하나가 보관할 수 있는 Emitter 정의의 최대 개수입니다.
     */
    constexpr Uint32 MaximumEmitters{256};

    /**
     * @brief Emitter 클래스 이름과 정의를 읽거나 기록합니다.
     * @param[in,out] Archive Emitter 레코드에 진입한 로드 또는 저장 아카이브입니다.
     * @param[in,out] Emitter 저장할 정의이며 로드 시 UClass 팩토리로 만든 객체를 받습니다.
     */
    void SerializeEmitter(FArchive& Archive, std::shared_ptr<UParticleEmitter>& Emitter) {
        FString Type{Archive.IsLoading() ? FString{} : Emitter->GetClass()->GetName()};

        Archive.Serialize("Type", Type);

        if (Archive.HasError()) {
            return;
        }

        if (Archive.IsLoading()) {
            const UClass* BaseClass{UParticleEmitter::StaticClass()};
            const UClass* Class{FReflectionRegistry::FindClass(Type)};

            if (Class == nullptr || !Class->IsChildOf(BaseClass)) {
                Archive.SetError("Unknown particle emitter class: " + Type);

                return;
            }

            std::shared_ptr<UObject> Object{Class->CreateInstance()};

            if (Cast<UParticleEmitter>(Object.get()) == nullptr) {
                Archive.SetError("Cannot create particle emitter class: " + Type);

                return;
            }

            Emitter = std::static_pointer_cast<UParticleEmitter>(Object);
        }

        Emitter->Serialize(Archive);
    }

}

/**
 * @brief UObject 이름을 ParticleSystem으로 설정하고 빈 Emitter 목록을 준비합니다.
 */
UParticleSystem::UParticleSystem() {
    SetName("ParticleSystem");
}

/**
 * @brief 자식 Emitter의 Outer 연결과 시스템이 가진 소유 참조를 해제합니다.
 */
UParticleSystem::~UParticleSystem() {
    ClearEmitters();
}

/**
 * @brief Emitter를 시스템에 추가하고 Outer를 이 시스템으로 연결합니다.
 */
bool UParticleSystem::AddEmitter(const std::shared_ptr<UParticleEmitter>& Emitter) {
    if (!Emitter || mEmitters.size() >= MaximumEmitters || std::find(mEmitters.begin(), mEmitters.end(), Emitter) != mEmitters.end()) {
        return false;
    }

    if ((Emitter->GetOuter() != nullptr && Emitter->GetOuter() != this) || !Emitter->SetOuter(this)) {
        return false;
    }

    mEmitters.push_back(Emitter);

    return true;
}

/**
 * @brief 지정한 Emitter의 Outer 연결과 목록 소유 참조를 제거합니다.
 */
bool UParticleSystem::RemoveEmitter(const UParticleEmitter* Emitter) {
    for (auto Iterator{mEmitters.begin()}; Iterator != mEmitters.end(); ++Iterator) {
        if (Iterator->get() == Emitter) {
            if ((*Iterator)->GetOuter() == this) {
                (*Iterator)->SetOuter(nullptr);
            }

            mEmitters.erase(Iterator);

            return true;
        }
    }

    return false;
}

/**
 * @brief 모든 자식 Emitter의 Outer 연결을 끊고 시스템의 소유 목록을 비웁니다.
 */
void UParticleSystem::ClearEmitters() {
    for (const auto& Emitter : mEmitters) {
        if (Emitter->GetOuter() == this) {
            Emitter->SetOuter(nullptr);
        }
    }

    mEmitters.clear();
}

/**
 * @brief UObject 자체를 이동하지 않고 다른 시스템의 Emitter 목록만 가져옵니다.
 */
void UParticleSystem::MoveEmittersFrom(UParticleSystem& Source) {
    if (&Source == this) {
        return;
    }

    ClearEmitters();
    mEmitters = std::move(Source.mEmitters);

    for (const auto& Emitter : mEmitters) {
        Emitter->SetOuter(this);
    }
}

/**
 * @brief 인덱스에 해당하는 Emitter의 수정 가능한 비소유 주소를 반환합니다.
 */
UParticleEmitter* UParticleSystem::GetEmitter(Core::Index_t Index) {
    return Index >= 0 && Index < mEmitters.size() ? mEmitters[Index].get() : nullptr;
}

/**
 * @brief 인덱스에 해당하는 Emitter의 읽기 전용 비소유 주소를 반환합니다.
 */
const UParticleEmitter* UParticleSystem::GetEmitter(Core::Index_t Index) const {
    return Index >= 0 && Index < mEmitters.size() ? mEmitters[Index].get() : nullptr;
}

/**
 * @brief 시스템이 보관하는 Emitter 정의의 개수를 반환합니다.
 */
std::size_t UParticleSystem::GetEmitterCount() const {
    return mEmitters.size();
}

/**
 * @brief Emitter 정의를 공유 소유하는 목록을 읽기 전용으로 제공합니다.
 */
const TArray<std::shared_ptr<UParticleEmitter>>& UParticleSystem::GetEmitters() const {
    return mEmitters;
}

/**
 * @brief 시스템 형식 정보와 Emitter 클래스·설정 목록을 아카이브로 읽거나 기록합니다.
 */
void UParticleSystem::Serialize(FArchive& Archive) {
    if (Archive.HasError()) {
        return;
    }

    FString Type{"ParticleSystem"};
    Uint32 Version{ParticleSystemFormatVersion};

    Archive.Serialize("Type", Type);
    Archive.Serialize("FormatVersion", Version);

    if (Type != "ParticleSystem" || Version != ParticleSystemFormatVersion) {
        Archive.SetError("Unsupported particle system type or version");
    }

    Uint32 Count{static_cast<Uint32>(mEmitters.size())};

    if (!Archive.BeginArray("Emitters", Count)) {
        return;
    }

    if (Count > MaximumEmitters) {
        Archive.SetError("Too many particle emitters");
    }

    TArray<std::shared_ptr<UParticleEmitter>> Emitters{};

    for (Core::Index_t Index{}; Index < Count && !Archive.HasError(); ++Index) {
        if (!Archive.BeginObject(std::to_string(Index))) {
            break;
        }

        std::shared_ptr<UParticleEmitter> Emitter{Archive.IsLoading() ? nullptr : mEmitters[Index]};

        SerializeEmitter(Archive, Emitter);
        Archive.EndObject();

        if (Archive.IsLoading() && !Archive.HasError()) {
            Emitters.push_back(std::move(Emitter));
        }
    }

    Archive.EndArray();

    if (Archive.IsLoading() && !Archive.HasError()) {
        ClearEmitters();
        mEmitters = std::move(Emitters);

        for (const auto& Emitter : mEmitters) {
            Emitter->SetOuter(this);
        }
    }
}
