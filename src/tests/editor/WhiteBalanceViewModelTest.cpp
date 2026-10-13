// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "core/effects/WhiteBalanceEffect.h"
#include "editor/Document.h"
#include "editor/viewmodel/WhiteBalanceViewModel.h"

#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

namespace {

using testutils::FakeStore;

constexpr int WAIT_MS = 5000;

/// A warm grey picture: a white balance picked on it cools it.
class WarmReader : public testutils::FakeReader
{
public:
    core::Image read(const std::filesystem::path &) const override
    {
        return testutils::uniform(100, 100, 0.6f, 0.5f, 0.35f);
    }
};

struct WhiteBalanceViewModelTest : ::testing::Test
{
    Document document{
        std::make_shared<WarmReader>(),
        std::make_shared<FakeStore>(),
        core::makeStandardPipeline()->parameters()};
    WhiteBalanceViewModel viewModel{&document};

    void open()
    {
        QSignalSpy opened{&document, &Document::opened};
        document.open("/pictures/a.tif");
        ASSERT_TRUE(QTest::qWaitFor([&] { return !opened.isEmpty(); }, WAIT_MS));
    }
};

} // namespace

TEST_F(WhiteBalanceViewModelTest, SetsTheTemperatureAndTheTintAndKeepsTheOtherValues)
{
    open();
    core::Settings own;
    own.setValue("Contrast2012", 20);
    document.setSettings(own);

    QSignalSpy picked{&viewModel, &WhiteBalanceViewModel::picked};
    viewModel.pick(QPointF{50, 50}, 5);
    ASSERT_TRUE(QTest::qWaitFor([&] { return !picked.isEmpty(); }, WAIT_MS));
    EXPECT_FALSE(picked.first().at(0).toString().isEmpty());

    // Warm: the pick cools it.
    EXPECT_LT(document.settings().value(core::WhiteBalanceEffect::TEMPERATURE), 0);
    EXPECT_DOUBLE_EQ(document.settings().values().at("Contrast2012"), 20);
}

TEST_F(WhiteBalanceViewModelTest, ExplainsAPickOutsideThePicture)
{
    open();
    QSignalSpy failed{&viewModel, &WhiteBalanceViewModel::failed};
    viewModel.pick(QPointF{-50, -50}, 5);
    ASSERT_TRUE(QTest::qWaitFor([&] { return !failed.isEmpty(); }, WAIT_MS));
    EXPECT_TRUE(document.settings().values().empty());
}

TEST_F(WhiteBalanceViewModelTest, PicksNothingWithoutAPicture)
{
    QSignalSpy picked{&viewModel, &WhiteBalanceViewModel::picked};
    QSignalSpy failed{&viewModel, &WhiteBalanceViewModel::failed};
    viewModel.pick(QPointF{50, 50}, 5);
    QTest::qWait(100);
    EXPECT_TRUE(picked.isEmpty());
    EXPECT_TRUE(failed.isEmpty());
}
