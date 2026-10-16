#include "ClientImageDescription.hpp"
#include "../../output/Monitor.hpp"

#include <algorithm>
#include <cmath>

using namespace NColorManagement;

PImageDescription NColorManagement::withSDRReferenceWhite(PImageDescription output, double whiteLuminance) {
    const auto& desc = output->value();
    if (desc.icc.present || desc.transferFunction != CM_TRANSFER_FUNCTION_ST2084_PQ || !std::isfinite(whiteLuminance) || whiteLuminance <= 0)
        return output;

    auto luminances = desc.luminances;
    // Reference white must fit in the target volume. Keep all HDR encoding and
    // mastering metadata intact; only describe where the compositor puts SDR white.
    const double peak    = desc.masteringLuminances.max > 0 ? desc.masteringLuminances.max : luminances.max;
    const double minimum = std::max(1.0, std::floor(luminances.min) + 1.0);
    if (peak < minimum)
        return output;

    luminances.reference = sc<uint32_t>(std::round(std::clamp(whiteLuminance, minimum, peak)));
    return output->with(luminances);
}

PImageDescription NColorManagement::clientImageDescription(PHLMONITORREF monitor) {
    const auto output = monitor->m_imageDescription;
    if (output->value().transferFunction != CM_TRANSFER_FUNCTION_ST2084_PQ || output->value().icc.present)
        return output;

    // SDR brightness is a linear luminance gain in the FP16 HDR work buffer.
    // The legacy non-FP16 path applies it in encoded space and is intentionally
    // left alone rather than reporting an incorrect linear gain for that path.
    const auto workTF = monitor->workBufferImageDescription()->value().transferFunction;
    if (workTF != CM_TRANSFER_FUNCTION_LINEAR && workTF != CM_TRANSFER_FUNCTION_EXT_LINEAR)
        return output;

    const double brightness = monitor->m_sdrBrightness > 0 ? monitor->m_sdrBrightness : 1.0;
    return withSDRReferenceWhite(output, monitor->m_sdrMaxLuminance * brightness);
}
