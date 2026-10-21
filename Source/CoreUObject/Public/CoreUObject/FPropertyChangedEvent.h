#pragma once

class FProperty;

struct FPropertyChangedEvent {
    const FProperty* mProperty{};
    bool mInteractive{};
};
