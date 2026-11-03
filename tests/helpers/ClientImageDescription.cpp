#include <helpers/cm/ClientImageDescription.hpp>
#include <gtest/gtest.h>

#include <limits>

using namespace NColorManagement;

static PImageDescription hdrOutput() {
    return CImageDescription::from(SImageDescription{
        .transferFunction    = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .primariesNameSet    = true,
        .primariesNamed      = CM_PRIMARIES_BT2020,
        .primaries           = NColorPrimaries::BT2020,
        .luminances          = {.min = 0.0969F, .max = 486, .reference = 203},
        .masteringLuminances = {.min = 0.0969F, .max = 486},
        .maxCLL              = 486,
        .maxFALL             = 486,
    });
}

TEST(ClientImageDescription, MatchesConfiguredSDRWhiteWithoutChangingScanout) {
    const auto output = hdrOutput();
    const auto client = withSDRReferenceWhite(output, 251.0 * 1.2);
    EXPECT_EQ(client->value().luminances.reference, 301U);
    EXPECT_EQ(output->value().luminances.reference, 203U);
    auto expected                 = output->value();
    expected.luminances.reference = 301;
    EXPECT_TRUE(client->value() == expected);
    EXPECT_GT(client->value().masteringLuminances.max, client->value().luminances.reference);
}

TEST(ClientImageDescription, SDRWhiteChangesProduceNewDescriptionIdentities) {
    const auto output = hdrOutput();
    const auto first  = withSDRReferenceWhite(output, 251);
    const auto second = withSDRReferenceWhite(output, 301);
    EXPECT_NE(first->id(), second->id());
    EXPECT_EQ(first->id(), withSDRReferenceWhite(output, 251)->id());
}

TEST(ClientImageDescription, ClampsToDisplayPeak) {
    const auto output = hdrOutput();
    EXPECT_EQ(withSDRReferenceWhite(output, 1000)->value().luminances.reference, 486U);
    EXPECT_EQ(withSDRReferenceWhite(output, 0.1)->value().luminances.reference, 1U);
}

TEST(ClientImageDescription, InvalidWhiteDoesNotChangeDescription) {
    const auto output = hdrOutput();
    for (const double value : {0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        EXPECT_EQ(withSDRReferenceWhite(output, value), output);
}

TEST(ClientImageDescription, SDRAndHLGDescriptionsRemainUnchanged) {
    const auto sdr            = CImageDescription::from(SImageDescription{});
    auto       hlgValue       = hdrOutput()->value();
    hlgValue.transferFunction = CM_TRANSFER_FUNCTION_HLG;
    const auto hlg            = CImageDescription::from(hlgValue);
    EXPECT_EQ(withSDRReferenceWhite(sdr, 301), sdr);
    EXPECT_EQ(withSDRReferenceWhite(hlg, 301), hlg);
}

TEST(ClientImageDescription, ICCDescriptionRemainsUnchanged) {
    auto value        = hdrOutput()->value();
    value.icc.present = true;
    const auto output = CImageDescription::from(value);
    EXPECT_EQ(withSDRReferenceWhite(output, 301), output);
}
