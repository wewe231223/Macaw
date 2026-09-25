#pragma once

#include <memory>

#include "SWindow.h"

class FSplitterRatio {
public:
    explicit FSplitterRatio(float InValue = 0.5f);

    float GetValue() const;
    void SetValue(float InValue);
    bool SharesStateWith(const FSplitterRatio& Other) const;

private:
    std::shared_ptr<float> mValue{};
};

class SSplitter : public SWindow {
public:
    void SetChildren(SWindow* InFirst, SWindow* InSecond);

    void SetRatio(float InRatio);

    float GetRatio() const;

    void SetRatioState(const FSplitterRatio& InRatio);

    const FSplitterRatio& GetRatioState() const;

    const FRect& GetHandleRect() const;

    virtual void DragTo(FPoint Point) = 0;

protected:
    virtual void UpdateLayout() = 0;

    SWindow* mFirst{nullptr};
    SWindow* mSecond{nullptr};
    FRect mHandleRect{};
    Int32 mHandleThickness{6};
    FSplitterRatio mRatio{};
    Int32 mMinimumSideSize{100};
};

// 수평 분할: 첫 번째 자식은 위, 두 번째 자식은 아래에 배치한다.
class SSplitterH final : public SSplitter {
public:
    void SetRect(const FRect& InRect) override;
    void DragTo(FPoint Point) override;

private:
    void UpdateLayout() override;
};

// 수직 분할: 첫 번째 자식은 왼쪽, 두 번째 자식은 오른쪽에 배치한다.
class SSplitterV final : public SSplitter {
public:
    void SetRect(const FRect& InRect) override;
    void DragTo(FPoint Point) override;

private:
    void UpdateLayout() override;
};
