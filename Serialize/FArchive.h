#pragma once
#include "../Core/Base/FGuid.h"
#include "../FName.h"

enum class EArchiveMode : Uint8 {
    Loading,
    Saving,
    Hashing,
    Counting
};

class FAssetRegistry;

class FArchive {
public:
    explicit FArchive(EArchiveMode InMode);

    virtual ~FArchive() = default;

    bool IsLoading() const;

    bool IsSaving() const;

    bool IsHashing() const;

    bool IsCounting() const;

    // ---------------------------------------------------
    // 1. 기본 원시 타입 (Primitives)
    // ---------------------------------------------------
    virtual void Serialize(std::string_view Name, bool& Value) = 0;
    virtual void Serialize(std::string_view Name, Uint8& Value) = 0;
    virtual void Serialize(std::string_view Name, Int32& Value) = 0;
    virtual void Serialize(std::string_view Name, Uint32& Value) = 0;
    virtual void Serialize(std::string_view Name, Int64& Value) = 0;
    virtual void Serialize(std::string_view Name, Uint64& Value) = 0;
    virtual void Serialize(std::string_view Name, Float32& Value) = 0;
    virtual void Serialize(std::string_view Name, Float64& Value) = 0;

    // ---------------------------------------------------
    // 2. 수학 코어 타입 (DirectX SimpleMath)
    // ---------------------------------------------------
    virtual void Serialize(std::string_view Name, FVector2& Value) = 0;
    virtual void Serialize(std::string_view Name, FVector& Value) = 0;
    virtual void Serialize(std::string_view Name, FVector4& Value) = 0;
    virtual void Serialize(std::string_view Name, FQuat& Value) = 0;
    virtual void Serialize(std::string_view Name, FMatrix& Value) = 0;

    // ---------------------------------------------------
    // 3. 엔진 코어 타입
    // ---------------------------------------------------
    virtual void Serialize(std::string_view Name, FString& Value) = 0;
    virtual void Serialize(std::string_view Name, FGuid& Value) = 0;

    template <typename T> void Serialize(std::string_view Name, TArray<T>& ArrayValue);

    void Serialize(std::string_view Name, FName& Value);

    // ---------------------------------------------------
    // 4. 구조체 / 계층 처리 (Scope)
    // ---------------------------------------------------
    virtual void BeginObjectScope(std::string_view Name) = 0;
    virtual void EndObjectScope() = 0;

    // ---------------------------------------------------
    // 5. 배열 처리 (Scope)
    // ---------------------------------------------------
    virtual void BeginArrayScope(std::string_view Name, std::size_t& ArraySize) = 0;
    virtual void EndArrayScope() = 0;

    // ---------------------------------------------------
    // 6. 구조체 직렬화 헬퍼 (UObject 자식들이나 커스텀 구조체)
    // ---------------------------------------------------
    template <typename T> void SerializeStruct(std::string_view Name, T& StructValue);

    // ---------------------------------------------------
    // 7. Asset Registry 처리
    // ---------------------------------------------------
    void SetAssetRegistry(FAssetRegistry* InputAssetRegistry);

    FAssetRegistry* GetAssetRegistry();

protected:
    const EArchiveMode Mode{};

private:
    FAssetRegistry* mAssetRegistry{};
};

template <typename T> void FArchive::Serialize(std::string_view Name, TArray<T>& ArrayValue) {
    std::size_t Size{ArrayValue.size()};

    BeginArrayScope(Name, Size);

    if (IsLoading()) {
        ArrayValue.resize(Size);
    }

    for (std::size_t I{0}; I < Size; ++I) {
        // 인덱스를 Name으로 변환하여 순회
        Serialize(std::to_string(I), ArrayValue[I]);
    }

    EndArrayScope();
}

template <typename T> void FArchive::SerializeStruct(std::string_view Name, T& StructValue) {
    BeginObjectScope(Name);
    StructValue.Serialize(*this);
    EndObjectScope();
}
