// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "imageio/Error.h"
#include "imageio/Files.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

void writeText(const std::filesystem::path &path, const std::string &text)
{
    std::ofstream{path, std::ios::binary} << text;
}

std::string readText(const std::filesystem::path &path)
{
    std::ifstream in{path, std::ios::binary};
    return {std::istreambuf_iterator<char>{in}, {}};
}

/// How many files `folder` holds.
std::size_t filesIn(const std::filesystem::path &folder)
{
    return static_cast<std::size_t>(std::distance(
        std::filesystem::directory_iterator{folder}, std::filesystem::directory_iterator{}));
}

} // namespace

TEST(FilesTest, EachWriteHasAHiddenTemporaryFileOfItsOwnBesideTheFile)
{
    const std::filesystem::path path = "/pictures/a.tif";
    const auto first = imageio::files::temporaryFor(path);
    const auto second = imageio::files::temporaryFor(path);
    EXPECT_NE(first, second);
    EXPECT_EQ(first.parent_path(), path.parent_path());
    EXPECT_TRUE(first.filename().string().starts_with(".a.tif."));
    EXPECT_EQ(first.extension(), ".part");
}

TEST(FilesTest, ReplacesAFileWholeAndLeavesNoTemporaryFile)
{
    const auto folder = testutils::outputFolder();
    const auto path = folder / "a.xmp";
    writeText(path, "old");
    const auto temporary = imageio::files::temporaryFor(path);
    writeText(temporary, "new");

    imageio::files::replace(temporary, path);
    EXPECT_EQ(readText(path), "new");
    EXPECT_EQ(filesIn(folder), 1u);
}

TEST(FilesTest, PutsANewFileInPlace)
{
    const auto folder = testutils::outputFolder();
    const auto path = folder / "a.xmp";
    const auto temporary = imageio::files::temporaryFor(path);
    writeText(temporary, "new");

    imageio::files::replace(temporary, path);
    EXPECT_EQ(readText(path), "new");
    EXPECT_EQ(filesIn(folder), 1u);
}

TEST(FilesTest, AFailureLeavesWhatWasThereAndNoTemporaryFile)
{
    const auto folder = testutils::outputFolder();
    // A folder of that name: no file can take its place.
    const auto path = folder / "a.tif";
    std::filesystem::create_directory(path);
    writeText(path / "inside", "kept");
    const auto temporary = imageio::files::temporaryFor(path);
    writeText(temporary, "new");

    EXPECT_THROW(imageio::files::replace(temporary, path), imageio::Error);
    EXPECT_EQ(readText(path / "inside"), "kept");
    EXPECT_FALSE(std::filesystem::exists(temporary));
    EXPECT_EQ(filesIn(folder), 1u);
}
