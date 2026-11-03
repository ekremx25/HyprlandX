#pragma once

#include "ColorManagement.hpp"
#include "../../desktop/DesktopTypes.hpp"

namespace NColorManagement {
    // Client-facing metadata only: do not use as the scanout or work-buffer description.
    PImageDescription withSDRReferenceWhite(PImageDescription output, double whiteLuminance);
    PImageDescription clientImageDescription(PHLMONITORREF monitor);
}
