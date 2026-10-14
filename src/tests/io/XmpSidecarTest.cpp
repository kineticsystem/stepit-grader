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

// What a raw editor writes next to a raw file, cut down: settings as attributes, and other
// metadata as elements.
constexpr const char *EDITOR_SIDECAR = R"(<x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="XMP Core 7.0">
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
   crs:Sharpness="40"
   crs:SharpenRadius="+1.5">
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

TEST(XmpSidecarTest, FormatsValuesAsRawEditorsDo)
{
    EXPECT_EQ(imageio::xmp::format(ExposureEffect::EXPOSURE, 0.5), "+0.50");
    EXPECT_EQ(imageio::xmp::format(ExposureEffect::EXPOSURE, -1.25), "-1.25");
    EXPECT_EQ(imageio::xmp::format(ExposureEffect::EXPOSURE, 0), "0.00");
    EXPECT_EQ(imageio::xmp::format(core::SharpnessEffect::SHARPNESS, 40), "40");
    EXPECT_EQ(imageio::xmp::format(core::SharpnessEffect::RADIUS, 2.5), "2.5");
}

TEST(XmpSidecarTest, ReadsWhatAnotherEditorWrites)
{
    const core::Settings settings = imageio::xmp::parse(EDITOR_SIDECAR, parameters());
    EXPECT_DOUBLE_EQ(settings.value(ExposureEffect::EXPOSURE), 0.65);
    EXPECT_EQ(settings.values().at("Contrast2012"), -12);
    EXPECT_EQ(settings.values().at("Clarity2012"), 20);
    EXPECT_EQ(settings.values().at("Sharpness"), 40);
    EXPECT_DOUBLE_EQ(settings.value(core::SharpnessEffect::RADIUS), 1.5);
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
    const std::string updated = imageio::xmp::write(EDITOR_SIDECAR, settings, parameters());
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
}

TEST(XmpSidecarTest, ReplacesASidecarThatIsNotXmpAndKeepsItBeside)
{
    const imageio::XmpSidecar sidecar;
    const fs::path image = testutils::outputFolder() / "IMG_0003.tif";
    const fs::path path = sidecar.location(image);
    fs::remove(imageio::damagedCopy(path));
    std::ofstream{path} << "<not xml";

    core::Settings settings;
    settings.setValue("Exposure2012", 0.5);
    sidecar.save(image, settings, parameters());
    const auto loaded = sidecar.load(image, parameters());
    ASSERT_TRUE(loaded.has_value());
    EXPECT_DOUBLE_EQ(loaded->value(ExposureEffect::EXPOSURE), 0.5);

    // The damaged one is kept, as it was, for what it may still hold.
    ASSERT_TRUE(fs::exists(imageio::damagedCopy(path)));
    std::ifstream copy{imageio::damagedCopy(path)};
    const std::string kept{std::istreambuf_iterator<char>{copy}, {}};
    EXPECT_EQ(kept, "<not xml");
    EXPECT_EQ(imageio::damagedCopy(path).filename(), "IMG_0003.tif.xmp.damaged");
}

TEST(XmpSidecarTest, LoadsTheSettingsOfAFileOfAnyName)
{
    const fs::path folder = testutils::outputFolder();
    const fs::path file = folder / "warm look.xmp";
    std::ofstream{file} << EDITOR_SIDECAR;
    const imageio::XmpSidecar sidecar;
    EXPECT_EQ(sidecar.extension(), "xmp");
    EXPECT_DOUBLE_EQ(sidecar.loadFile(file, parameters()).value(ExposureEffect::EXPOSURE), 0.65);
    EXPECT_THROW(sidecar.loadFile(folder / "missing.xmp", parameters()), imageio::Error);
}

TEST(XmpSidecarTest, ARawFileAndATiffOfTheSameNameEachHaveTheirOwn)
{
    const imageio::XmpSidecar sidecar;
    EXPECT_EQ(sidecar.location("/pictures/IMG_0001.CR2"), fs::path{"/pictures/IMG_0001.xmp"});
    EXPECT_EQ(sidecar.location("/pictures/IMG_0001.dng"), fs::path{"/pictures/IMG_0001.xmp"});
    EXPECT_EQ(sidecar.location("/pictures/IMG_0001.tif"), fs::path{"/pictures/IMG_0001.tif.xmp"});
    EXPECT_EQ(sidecar.location("/pictures/IMG_0001.tiff"), fs::path{"/pictures/IMG_0001.tiff.xmp"});
}

TEST(XmpSidecarTest, ATiffReadsItsFormerSidecarUnlessARawFileOwnsIt)
{
    const imageio::XmpSidecar sidecar;
    const fs::path folder = testutils::outputFolder() / "former-sidecar";
    fs::remove_all(folder);
    fs::create_directories(folder);
    const fs::path tiff = folder / "angle_18.tif";
    std::ofstream{folder / "angle_18.xmp"} << EDITOR_SIDECAR;

    // Saved under the old name, before a TIFF's sidecar took its whole name: read from there.
    ASSERT_TRUE(sidecar.formerLocation(tiff).has_value());
    const auto former = sidecar.load(tiff, parameters());
    ASSERT_TRUE(former.has_value());
    EXPECT_DOUBLE_EQ(former->value(ExposureEffect::EXPOSURE), 0.65);

    // Saved under the new name, the old file left as it is; the new one is read from then on.
    core::Settings settings;
    settings.setValue("Exposure2012", -1.0);
    sidecar.save(tiff, settings, parameters());
    EXPECT_TRUE(fs::exists(folder / "angle_18.tif.xmp"));
    EXPECT_DOUBLE_EQ(sidecar.load(tiff, parameters())->value(ExposureEffect::EXPOSURE), -1.0);
    EXPECT_DOUBLE_EQ(
        sidecar.load(folder / "angle_18.CR2", parameters())->value(ExposureEffect::EXPOSURE), 0.65);

    // With a raw file of the same name beside it, the old file is the raw file's: not read.
    const fs::path other = folder / "IMG_0001.tif";
    std::ofstream{folder / "IMG_0001.xmp"} << EDITOR_SIDECAR;
    std::ofstream{folder / "IMG_0001.CR2"} << "raw";
    EXPECT_FALSE(sidecar.formerLocation(other).has_value());
    EXPECT_FALSE(sidecar.load(other, parameters()).has_value());
    EXPECT_FALSE(sidecar.formerLocation(folder / "IMG_0001.CR2").has_value());
}
