#include "pch.h"
#include "Render/FViewElementCollector.h"

FViewElementCollector::FViewElementCollector(const FRenderView& View)
	: mView(View) {
}

void FViewElementCollector::DrawText(const FTextDrawData& Text, const FMatrix& World) {
    if (mView.IsPassEnabled(ERenderPass::Text)) {
        mTexts.push_back(Text);
        mTexts.back().mWorld = World;
    }
}

void FViewElementCollector::DrawBillboard(const FBillboardDrawData& Billboard, const FMatrix& World) {
    if (mView.IsPassEnabled(ERenderPass::Billboard)) {
        mBillboards.push_back(Billboard);
        mBillboards.back().mWorld = World;
    }
}

const TArray<FTextDrawData>& FViewElementCollector::GetTextDraws() const {
    return mTexts;
}

const TArray<FBillboardDrawData>& FViewElementCollector::GetBillboardDraws() const {
    return mBillboards;
}
