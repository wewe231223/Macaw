#ifndef MacawLightDataIncluded
#define MacawLightDataIncluded

struct FLight {
    float4 mPositionAndRadius;
    float4 mDirectionAndType;
    float4 mColorAndIntensity;
    float4 mFalloffAndCones;
    float4 mShadowInfo;
};

#endif
