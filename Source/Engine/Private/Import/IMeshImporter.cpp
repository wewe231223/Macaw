#include "Engine/Import/IMeshImporter.h"
#include "Engine/Import/FAssimpMeshImporter.h"

IMeshImporter::~IMeshImporter() = default;

const IMeshImporter& IMeshImporter::GetDefault() {
    static const FAssimpMeshImporter Importer{};
    return Importer;
}
