// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/WaitingDialog.h"

#include <QCloseEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

/// The width of the window, in characters: the messages fit without the window resizing.
constexpr int MESSAGE_CHARACTERS = 60;

} // namespace

WaitingDialog::WaitingDialog(
    std::shared_ptr<StatusController> controller, const QString &stopText, QWidget *parent)
    : QDialog{parent}
    , m_controller{std::move(controller)}
    , m_message{new QLabel{this}}
    , m_progress{new QProgressBar{this}}
    , m_stop{new QPushButton{stopText, this}}
{
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_message->setMinimumWidth(
        QFontMetrics{font()}.horizontalAdvance(QString(MESSAGE_CHARACTERS, 'x')));

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_stop);
    auto *layout = new QVBoxLayout{this};
    layout->addWidget(m_message);
    layout->addWidget(m_progress);
    layout->addLayout(buttons);

    connect(m_stop, &QPushButton::clicked, this, &WaitingDialog::stop);
    connect(m_controller.get(), &StatusController::status, this, [this](const Status &status) {
        m_message->setText(status.message);
        m_progress->setValue(status.value);
    });
}

QString WaitingDialog::message() const
{
    return m_message->text();
}

void WaitingDialog::stop()
{
    m_stop->setEnabled(false);
    m_controller->stop();
}

void WaitingDialog::closeEvent(QCloseEvent *event)
{
    // The window's close button stops the task as the Stop button does; the window stays until
    // the task ends, which closes it.
    if (!m_controller->isStopped()) {
        stop();
    }
    event->ignore();
}
