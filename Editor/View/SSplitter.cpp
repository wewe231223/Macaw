#include "pch.h"

#include "Editor/View/SSplitter.h"

FSplitterRatio::FSplitterRatio(float InValue)
    : mValue(std::make_shared<float>(std::clamp(InValue, 0.0f, 1.0f))) {
}

float FSplitterRatio::GetValue() const {
    return *mValue;
}

void FSplitterRatio::SetValue(float InValue) {
    *mValue = std::clamp(InValue, 0.0f, 1.0f);
}

bool FSplitterRatio::SharesStateWith(const FSplitterRatio& Other) const {
    return mValue == Other.mValue;
}

void SSplitter::SetRatio(float InRatio) {
    mRatio.SetValue(InRatio);
    UpdateLayout();
}

void SSplitter::SetRatioState(const FSplitterRatio& InRatio) {
    mRatio = InRatio;
}

void SSplitterH::SetRect(const FRect& InRect) {
    SWindow::SetRect(InRect);
    UpdateLayout();
}

void SSplitterH::DragTo(FPoint Point) {
    const Int32 AvailableHeight{std::max(0, mRect.GetHeight() - mHandleThickness)};
    if (AvailableHeight == 0) {
        return;
    }

    mRatio.SetValue((static_cast<float>(Point.mY - mRect.mMin.mY) - mHandleThickness * 0.5f) / static_cast<float>(AvailableHeight));
    UpdateLayout();
}

void SSplitterH::UpdateLayout() {
    if (mFirst == nullptr || mSecond == nullptr) {
        mHandleRect = {};
        return;
    }

    const Int32 Thickness{std::clamp(mHandleThickness, 0, std::max(0, mRect.GetHeight()))};
    const Int32 AvailableHeight{std::max(0, mRect.GetHeight() - Thickness)};
    const Int32 Minimum{std::min(mMinimumSideSize, AvailableHeight / 2)};
    const Int32 FirstHeight{std::clamp(static_cast<Int32>(std::round(AvailableHeight * mRatio.GetValue())), Minimum, AvailableHeight - Minimum)};
    mRatio.SetValue(AvailableHeight > 0 ? static_cast<float>(FirstHeight) / AvailableHeight : 0.5f);

    const Int32 SplitY{mRect.mMin.mY + FirstHeight};
    mHandleRect = {{mRect.mMin.mX, SplitY}, {mRect.mMax.mX, SplitY + Thickness}};
    mFirst->SetRect({mRect.mMin, {mRect.mMax.mX, mHandleRect.mMin.mY}});
    mSecond->SetRect({{mRect.mMin.mX, mHandleRect.mMax.mY}, mRect.mMax});
}

void SSplitterV::SetRect(const FRect& InRect) {
    SWindow::SetRect(InRect);
    UpdateLayout();
}

void SSplitterV::DragTo(FPoint Point) {
    const Int32 AvailableWidth{std::max(0, mRect.GetWidth() - mHandleThickness)};
    if (AvailableWidth == 0) {
        return;
    }

    mRatio.SetValue((static_cast<float>(Point.mX - mRect.mMin.mX) - mHandleThickness * 0.5f) / static_cast<float>(AvailableWidth));
    UpdateLayout();
}

void SSplitterV::UpdateLayout() {
    if (mFirst == nullptr || mSecond == nullptr) {
        mHandleRect = {};
        return;
    }

    const Int32 Thickness{std::clamp(mHandleThickness, 0, std::max(0, mRect.GetWidth()))};
    const Int32 AvailableWidth{std::max(0, mRect.GetWidth() - Thickness)};
    const Int32 Minimum{std::min(mMinimumSideSize, AvailableWidth / 2)};
    const Int32 FirstWidth{std::clamp(static_cast<Int32>(std::round(AvailableWidth * mRatio.GetValue())), Minimum, AvailableWidth - Minimum)};
    mRatio.SetValue(AvailableWidth > 0 ? static_cast<float>(FirstWidth) / AvailableWidth : 0.5f);

    const Int32 SplitX{mRect.mMin.mX + FirstWidth};
    mHandleRect = {{SplitX, mRect.mMin.mY}, {SplitX + Thickness, mRect.mMax.mY}};
    mFirst->SetRect({mRect.mMin, {mHandleRect.mMin.mX, mRect.mMax.mY}});
    mSecond->SetRect({{mHandleRect.mMax.mX, mRect.mMin.mY}, mRect.mMax});
}

void SSplitter::SetChildren(SWindow* InFirst, SWindow* InSecond) {
    mFirst = InFirst;
    mSecond = InSecond;
}

float SSplitter::GetRatio() const {
    return mRatio.GetValue();
}

const FSplitterRatio& SSplitter::GetRatioState() const {
    return mRatio;
}

const FRect& SSplitter::GetHandleRect() const {
    return mHandleRect;
}
