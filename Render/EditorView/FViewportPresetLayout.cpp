#include "pch.h"

#include "FViewportPresetLayout.h"

FViewportPresetLayout::FViewportPresetLayout(EViewportLayoutPreset InPreset) {
    SetPreset(InPreset);
}

void FViewportPresetLayout::SetPreset(EViewportLayoutPreset InPreset) {
    Reset();
    mPreset = InPreset < EViewportLayoutPreset::Count ? InPreset : EViewportLayoutPreset::Single;

    switch (mPreset) {
        case EViewportLayoutPreset::Single:
            mViewportCount = 1;
            mRoot = &mViewportRegions[0];
            break;

        case EViewportLayoutPreset::TwoTopBottom:
            mViewportCount = 2;
            mRoot = AddTopBottomSplitter(&mViewportRegions[0], &mViewportRegions[1]);
            SetDrawOrder({static_cast<SSplitter*>(mRoot)});
            break;

        case EViewportLayoutPreset::TwoLeftRight:
            mViewportCount = 2;
            mRoot = AddLeftRightSplitter(&mViewportRegions[0], &mViewportRegions[1]);
            SetDrawOrder({static_cast<SSplitter*>(mRoot)});
            break;

        case EViewportLayoutPreset::ThreeLeftOneRightTwo: {
            mViewportCount = 3;
            SSplitter* Right{AddTopBottomSplitter(&mViewportRegions[1], &mViewportRegions[2])};
            mRoot = AddLeftRightSplitter(&mViewportRegions[0], Right);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), Right});
            break;
        }

        case EViewportLayoutPreset::ThreeLeftTwoRightOne: {
            mViewportCount = 3;
            SSplitter* Left{AddTopBottomSplitter(&mViewportRegions[0], &mViewportRegions[1])};
            mRoot = AddLeftRightSplitter(Left, &mViewportRegions[2]);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), Left});
            break;
        }

        case EViewportLayoutPreset::ThreeTopOneBottomTwo: {
            mViewportCount = 3;
            SSplitter* Bottom{AddLeftRightSplitter(&mViewportRegions[1], &mViewportRegions[2])};
            mRoot = AddTopBottomSplitter(&mViewportRegions[0], Bottom);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), Bottom});
            break;
        }

        case EViewportLayoutPreset::ThreeTopTwoBottomOne: {
            mViewportCount = 3;
            SSplitter* Top{AddLeftRightSplitter(&mViewportRegions[0], &mViewportRegions[1])};
            mRoot = AddTopBottomSplitter(Top, &mViewportRegions[2]);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), Top});
            break;
        }

        case EViewportLayoutPreset::FourGrid: {
            mViewportCount = 4;
            mSharedGridRatio = FSplitterRatio(0.5f);
            SSplitter* Top{AddLeftRightSplitter(&mViewportRegions[0], &mViewportRegions[1], 0.5f, &mSharedGridRatio)};
            SSplitter* Bottom{AddLeftRightSplitter(&mViewportRegions[2], &mViewportRegions[3], 0.5f, &mSharedGridRatio)};
            mRoot = AddTopBottomSplitter(Top, Bottom);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), Top, Bottom});
            break;
        }

        case EViewportLayoutPreset::FourLeftOneRightThree: {
            mViewportCount = 4;
            SWindow* Right{BuildTopBottomThree(1, 2, 3)};
            mRoot = AddLeftRightSplitter(&mViewportRegions[0], Right);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), mSplitters[1].get(), mSplitters[0].get()});
            break;
        }

        case EViewportLayoutPreset::FourLeftThreeRightOne: {
            mViewportCount = 4;
            SWindow* Left{BuildTopBottomThree(0, 1, 2)};
            mRoot = AddLeftRightSplitter(Left, &mViewportRegions[3]);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), mSplitters[1].get(), mSplitters[0].get()});
            break;
        }

        case EViewportLayoutPreset::FourTopOneBottomThree: {
            mViewportCount = 4;
            SWindow* Bottom{BuildLeftRightThree(1, 2, 3)};
            mRoot = AddTopBottomSplitter(&mViewportRegions[0], Bottom);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), mSplitters[1].get(), mSplitters[0].get()});
            break;
        }

        case EViewportLayoutPreset::FourTopThreeBottomOne: {
            mViewportCount = 4;
            SWindow* Top{BuildLeftRightThree(0, 1, 2)};
            mRoot = AddTopBottomSplitter(Top, &mViewportRegions[3]);
            SetDrawOrder({static_cast<SSplitter*>(mRoot), mSplitters[1].get(), mSplitters[0].get()});
            break;
        }

        default:
            mViewportCount = 1;
            mRoot = &mViewportRegions[0];
            break;
    }

    SetRect(mRect);
}

EViewportLayoutPreset FViewportPresetLayout::GetPreset() const {
    return mPreset;
}

const char* FViewportPresetLayout::GetPresetName() const {
    return GetPresetName(mPreset);
}

const char* FViewportPresetLayout::GetPresetName(EViewportLayoutPreset InPreset) {
    switch (InPreset) {
        case EViewportLayoutPreset::Single:
            return "1: Single";
        case EViewportLayoutPreset::TwoTopBottom:
            return "2: Top | Bottom";
        case EViewportLayoutPreset::TwoLeftRight:
            return "2: Left | Right";
        case EViewportLayoutPreset::ThreeLeftOneRightTwo:
            return "3: Left 1 | Right 2";
        case EViewportLayoutPreset::ThreeLeftTwoRightOne:
            return "3: Left 2 | Right 1";
        case EViewportLayoutPreset::ThreeTopOneBottomTwo:
            return "3: Top 1 | Bottom 2";
        case EViewportLayoutPreset::ThreeTopTwoBottomOne:
            return "3: Top 2 | Bottom 1";
        case EViewportLayoutPreset::FourGrid:
            return "4: Grid";
        case EViewportLayoutPreset::FourLeftOneRightThree:
            return "4: Left 1 | Right 3";
        case EViewportLayoutPreset::FourLeftThreeRightOne:
            return "4: Left 3 | Right 1";
        case EViewportLayoutPreset::FourTopOneBottomThree:
            return "4: Top 1 | Bottom 3";
        case EViewportLayoutPreset::FourTopThreeBottomOne:
            return "4: Top 3 | Bottom 1";
        default:
            return "Unknown";
    }
}

Uint32 FViewportPresetLayout::GetViewportCount() const {
    return mViewportCount;
}

void FViewportPresetLayout::SetRect(const FRect& InRect) {
    mRect = InRect;
    if (mRoot != nullptr) {
        mRoot->SetRect(mRect);
    }
}

void FViewportPresetLayout::RefreshLayout() {
    SetRect(mRect);
}

const FRect& FViewportPresetLayout::GetViewportRect(FViewportId ViewportId) const {
    return mViewportRegions[ViewportId].GetRect();
}

void FViewportPresetLayout::CollectSplitters(std::vector<SSplitter*>& OutSplitters) const {
    for (Uint32 Index{0}; Index < mSplitterCount; ++Index) {
        OutSplitters.push_back(mSplitterDrawOrder[Index]);
    }
}

void FViewportPresetLayout::GetSplitterRatios(std::array<float, MaximumSplitterCount>& OutRatios, Uint32& OutCount) const {
    OutCount = mSplitterCount;

    for (Uint32 Index{0}; Index < mSplitterCount; ++Index) {
        OutRatios[Index] = mSplitterDrawOrder[Index]->GetRatio();
    }
}

bool FViewportPresetLayout::RestoreSplitterRatios(const std::array<float, MaximumSplitterCount>& Ratios, Uint32 RatioCount) {
    if (RatioCount != mSplitterCount || RatioCount > MaximumSplitterCount) {
        return false;
    }

    for (Uint32 Index{0}; Index < RatioCount; ++Index) {
        if (mSplitterDrawOrder[Index] == nullptr) {
            return false;
        }

        const float Ratio{Ratios[Index]};

        if (Ratio < 0.0f || Ratio > 1.0f) {
            return false;
        }
    }

    for (Uint32 Index{0}; Index < RatioCount; ++Index) {
        mSplitterDrawOrder[Index]->SetRatio(Ratios[Index]);
    }

    RefreshLayout();
    return true;
}

SSplitter* FViewportPresetLayout::AddTopBottomSplitter(SWindow* First, SWindow* Second, float Ratio, const FSplitterRatio* SharedRatio) {
    return AddSplitter(std::make_unique<SSplitterH>(), First, Second, Ratio, SharedRatio);
}

SSplitter* FViewportPresetLayout::AddLeftRightSplitter(SWindow* First, SWindow* Second, float Ratio, const FSplitterRatio* SharedRatio) {
    return AddSplitter(std::make_unique<SSplitterV>(), First, Second, Ratio, SharedRatio);
}

SSplitter* FViewportPresetLayout::AddSplitter(std::unique_ptr<SSplitter> Splitter, SWindow* First, SWindow* Second, float Ratio, const FSplitterRatio* SharedRatio) {
    if (mSplitterCount >= MaximumSplitterCount) {
        return nullptr;
    }

    Splitter->SetChildren(First, Second);
    Splitter->SetRatioState(SharedRatio != nullptr ? *SharedRatio : FSplitterRatio(Ratio));
    SSplitter* Result{Splitter.get()};
    mSplitters[mSplitterCount++] = std::move(Splitter);
    return Result;
}

SWindow* FViewportPresetLayout::BuildTopBottomThree(FViewportId FirstId, FViewportId SecondId, FViewportId ThirdId) {
    SSplitter* LastTwo{AddTopBottomSplitter(&mViewportRegions[SecondId], &mViewportRegions[ThirdId])};
    return AddTopBottomSplitter(&mViewportRegions[FirstId], LastTwo, 1.0f / 3.0f);
}

SWindow* FViewportPresetLayout::BuildLeftRightThree(FViewportId FirstId, FViewportId SecondId, FViewportId ThirdId) {
    SSplitter* LastTwo{AddLeftRightSplitter(&mViewportRegions[SecondId], &mViewportRegions[ThirdId])};
    return AddLeftRightSplitter(&mViewportRegions[FirstId], LastTwo, 1.0f / 3.0f);
}

void FViewportPresetLayout::Reset() {
    mRoot = nullptr;
    mSplitterCount = 0;
    mSplitterDrawOrder.fill(nullptr);

    for (std::unique_ptr<SSplitter>& Splitter : mSplitters) {
        Splitter.reset();
    }

    for (SWindow& Region : mViewportRegions) {
        Region.SetRect({});
    }
}

void FViewportPresetLayout::SetDrawOrder(std::initializer_list<SSplitter*> InSplitters) {
    Uint32 Index{0};
    for (SSplitter* Splitter : InSplitters) {
        mSplitterDrawOrder[Index++] = Splitter;
    }
}
