// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include <QApplication>

#include <gtest/gtest.h>

// One QApplication for every test: the widget tests need it, and Qt allows only one.
int main(int argc, char **argv)
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    QApplication app{argc, argv};
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
