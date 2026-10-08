#pragma once
#include "RenderCore/FRenderData.h"

class FDynamicPrimitiveDrawInterface {
public:
    virtual ~FDynamicPrimitiveDrawInterface();

public:
    virtual void DrawText(const FTextDrawData& Text, const FMatrix& World) = 0;
    virtual void DrawBillboard(const FBillboardDrawData& Billboard, const FMatrix& World) = 0;
};
