#pragma once

class FScopedFlag final {
public:
    explicit FScopedFlag(bool& Flag, bool Value = true);
    ~FScopedFlag();
    FScopedFlag(const FScopedFlag&) = delete;
    FScopedFlag& operator=(const FScopedFlag&) = delete;
    FScopedFlag(FScopedFlag&&) = delete;
    FScopedFlag& operator=(FScopedFlag&&) = delete;

private:
    bool& mFlag;
    bool mPreviousValue{};
};
