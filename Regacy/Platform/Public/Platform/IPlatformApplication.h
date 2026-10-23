#pragma once

class IPlatformApplication {
public:
    virtual ~IPlatformApplication() = default;

public:
    virtual bool PumpMessages() = 0;
    virtual int GetExitCode() const = 0;
};
