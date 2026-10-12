// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "core/effects/ExposureEffect.h"
#include "core/effects/SharpnessEffect.h"
#include "imageio/Error.h"
#include "imageio/XmpSidecar.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using core::ExposureEffect;

namespace {

const std::vector<core::Parameter> &parameters()
{
    static const auto all = core::makeStandardPipeline()->parameters();
    return all;
}

std::string read(const fs::path &path)
{
    std::ifstream in{path};
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

// What Lightroom Classic writes next to a raw file, cut down: settings as attributes, and other
// metadata as elements.
constexpr const char *LIGHTROOM_SIDECAR
    = R"(<x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="Adobe XMP Core 7.0-c000">
 <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
  <rdf:Description rdf:about=""
    xmlns:xmp="http://ns.adobe.com/xap/1.0/"
    xmlns:crs="http://ns.adobe.com/camera-raw-settings/1.0/"
   xmp:Rating="4"
   crs:Version="15.0"
   crs:ProcessVersion="11.0"
   crs:WhiteBalance="As Shot"
   crs:Exposure2012="+0.65"
   crs:Contrast2012="-12"
   crs:Clarity2012="+20"
   crs:Sharpness="40">
   <crs:ToneCurvePV2012>
    <rdf:Seq>
     <rdf:li>0, 0</rdf:li>
     <rdf:li>255, 255</rdf:li>
    </rdf:Seq>
   </crs:ToneCurvePV2012>
  </rdf:Description>
 </rdf:RDF>
</x:xmpmeta>
)";

} // namespace

TEST(XmpSidecarTest, FormatsValuesAsLightroomDoes)
{
    EXPECT_EQ(imageio::xmp::format(ExposureEffect::EXPOSURE, 0.5), "+0.50");
    EXPECT_EQ(imageio::xmp::format(ExposureEffect::EXPOSURE, -1.25), "-1.25");
    EXPECT_EQ(imageio::xmp::format(ExposureEffect::EXPOSURE, 0), "0.00");
    EXPECT_EQ(imageio::xmp::format(core::SharpnessEffect::SHARPNESS, 40), "40");
}

TEST(XmpSidecarTest, ReadsWhatLightroomWrites)
{
    const core::Settings settings = imageio::xmp::parse(LIGHTROOM_SIDECAR, parameters());
    EXPECT_DOUBLE_EQ(settings.value(ExposureEffect::EXPOSURE), 0.65);
    EXPECT_EQ(settings.values().at("Contrast2012"), -12);
    EXPECT_EQ(settings.values().at("Clarity2012"), 20);
    EXPECT_EQ(settings.values().at("Sharpness"), 40);
    EXPECT_FALSE(settings.has("Shadows2012"));
}

TEST(XmpSidecarTest, ReadsSettingsWrittenAsElementsWithAnyPrefix)
{
    const std::string document
        = R"(<x:xmpmeta xmlns:x="adobe:ns:meta/"><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
      <rdf:Description rdf:about="" xmlns:cr="http://ns.adobe.com/camera-raw-settings/1.0/">
        <cr:Exposure2012>-1.5</cr:Exposure2012>
      </rdf:Description></rdf:RDF></x:xmpmeta>)";
    EXPECT_DOUBLE_EQ(
        imageio::xmp::parse(document, parameters()).value(ExposureEffect::EXPOSURE), -1.5);
}

TEST(XmpSidecarTest, UpdatingKeepsEverythingElse)
{
    core::Settings settings;
    settings.setValue(ExposureEffect::EXPOSURE.id, 1.0);
    const std::string updated = imageio::xmp::write(LIGHTROOM_SIDECAR, settings, parameters());
    EXPECT_NE(updated.find(R"(xmp:Rating="4")"), std::string::npos);
    EXPECT_NE(updated.find("<crs:ToneCurvePV2012>"), std::string::npos);
    EXPECT_NE(updated.find(R"(crs:WhiteBalance="As Shot")"), std::string::npos);
    EXPECT_NE(updated.find(R"(crs:Exposure2012="+1.00")"), std::string::npos);
    EXPECT_NE(updated.find(R"(crs:Contrast2012="0")"), std::string::npos);
    EXPECT_EQ(updated.find("+0.65"), std::string::npos);
}

TEST(XmpSidecarTest, SavesNextToTheImageAndLoadsItBack)
{
    const fs::path image = testutils::outputFolder() / "IMG_0001.CR2";
    const imageio::XmpSidecar sidecar;
    EXPECT_EQ(sidecar.location(image).filename(), "IMG_0001.xmp");
    EXPECT_FALSE(sidecar.load(image, parameters()));

    core::Settings settings;
    settings.setValue(ExposureEffect::EXPOSURE.id, -0.35);
    settings.setValue("Shadows2012", 45);
    sidecar.save(image, settings, parameters());

    const auto loaded = sidecar.load(image, parameters());
    ASSERT_TRUE(loaded);
    EXPECT_TRUE(loaded->sameAs(settings, parameters()));
    const std::string text = read(sidecar.location(image));
    EXPECT_NE(text.find(R"(crs:ProcessVersion="11.0")"), std::string::npos);
    EXPECT_NE(text.find(R"(crs:HasSettings="True")"), std::string::npos);
}

TEST(XmpSidecarTest, ReportsASidecarThatIsNotXml)
{
    const fs::path image = testutils::outputFolder() / "IMG_0002.tif";
    std::ofstream{imageio::XmpSidecar{}.location(image)} << "<not xml";
    EXPECT_THROW(imageio::XmpSidecar{}.load(image, parameters()), imageio::Error);
    EXPECT_THROW(imageio::XmpSidecar{}.save(image, {}, parameters()), imageio::Error);
}
