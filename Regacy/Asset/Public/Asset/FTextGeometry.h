#pragma once

#include "RenderCore/FRenderData.h"

class UFont;
class IAssetRegistryMutator;

bool BuildTextGeometry(const UFont& Font, IAssetRegistryMutator& Registry, FAssetHandle FontHandle, const FString& Text, float CharacterHeight, float LetterSpacing, float LineSpacing, TArray<FTextVertex>& Vertices);
