#include "Core/FRevision.h"

namespace Core {

    Core::FRevision::FRevision(Core::Uint64 Value)
		: mValue{Value} {
    }

    Core::FRevision Core::FRevision::Next() {
        static Core::Uint64 Counter{};

        return Core::FRevision{++Counter};
    }

    bool Core::FRevision::IsValid() const {
        return mValue != 0;
    }

    Core::Uint64 Core::FRevision::GetValue() const {
        return mValue;
    }

}
