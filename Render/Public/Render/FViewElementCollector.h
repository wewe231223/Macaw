#pragma once
#include "RenderCore/FDynamicPrimitiveDrawInterface.h"
#include "Render/FRenderView.h"

class FViewElementCollector final : public FDynamicPrimitiveDrawInterface {
public:
    explicit FViewElementCollector(const FRenderView& View);

public:
    void DrawText(const FTextDrawData& Text, const FMatrix& World) override;
    void DrawBillboard(const FBillboardDrawData& Billboard, const FMatrix& World) override;
    const TArray<FTextDrawData>& GetTextDraws() const;
    const TArray<FBillboardDrawData>& GetBillboardDraws() const;

private:
    const FRenderView& mView;
    TArray<FTextDrawData> mTexts{};
    TArray<FBillboardDrawData> mBillboards{};
};
