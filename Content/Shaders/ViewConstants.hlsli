#ifndef MacawViewConstants
#define MacawViewConstants

cbuffer ViewConstants : register(b1) {
    row_major float4x4 ViewProjection;
};

#endif
