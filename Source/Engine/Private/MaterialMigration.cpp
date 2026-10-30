#include "Engine/MaterialMigration.h"
#include "Asset/FAssetManager.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <string_view>

namespace Engine::MaterialMigration {

    static bool MigrateSceneReferences(rapidjson::Value& Value, rapidjson::Document::AllocatorType& Allocator, const Asset::FAssetManager::FMaterialReferenceMigrator& MigrateReference, bool& Changed, Core::FString& Error, bool MaterialReference, Core::Uint32 Depth) {
        if (Depth > 32) {
            Error = "Scene nesting exceeds the migration limit";
            return false;
        }

        if (Value.IsObject()) {
            for (auto Position{Value.MemberBegin()}; Position != Value.MemberEnd(); ++Position) {
                const std::string_view Name{Position->name.GetString(), Position->name.GetStringLength()};

                if (!MigrateSceneReferences(Position->value, Allocator, MigrateReference, Changed, Error, Name == "Material" || Name == "SlotMaterials", Depth + 1)) {
                    return false;
                }
            }
        } else if (Value.IsArray()) {
            for (auto& Child : Value.GetArray()) {
                if (!MigrateSceneReferences(Child, Allocator, MigrateReference, Changed, Error, MaterialReference, Depth + 1)) {
                    return false;
                }
            }
        } else if (MaterialReference && Value.IsString()) {
            Core::FString Reference{Value.GetString(), Value.GetStringLength()};
            bool Replaced{};

            if (!MigrateReference(Reference, Replaced, Error)) {
                return false;
            }

            if (Replaced) {
                Value.SetString(Reference.data(), static_cast<rapidjson::SizeType>(Reference.size()), Allocator);
                Changed = true;
            }
        }

        return true;
    }

    static bool MigrateAssetReferences(Asset::EAssetType Type, Core::TArray<Core::Uint8>& Payload, const Asset::FAssetManager::FMaterialReferenceMigrator& MigrateReference, bool& Changed, Core::FString& Error) {
        if (Type != Asset::EAssetType::Scene) {
            return true;
        }

        rapidjson::Document Document{};
        Document.Parse<rapidjson::kParseValidateEncodingFlag>(reinterpret_cast<const char*>(Payload.data()), Payload.size());

        if (Document.HasParseError() || !Document.IsObject()) {
            Error = "Invalid scene data during material migration";
            return false;
        }

        if (!MigrateSceneReferences(Document, Document.GetAllocator(), MigrateReference, Changed, Error, false, 0)) {
            return false;
        }

        if (Changed) {
            rapidjson::StringBuffer Buffer{};
            rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer{Buffer};
            Document.Accept(Writer);
            const auto* Begin{reinterpret_cast<const Core::Uint8*>(Buffer.GetString())};
            Payload.assign(Begin, Begin + Buffer.GetSize());
        }

        return true;
    }

    bool Engine::MaterialMigration::Migrate(Asset::FAssetManager& Assets, Core::FString& Error) {
        return Assets.MigrateMaterials(&MigrateAssetReferences, Error);
    }

}
