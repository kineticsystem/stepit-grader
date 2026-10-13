// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/viewmodel/WhiteBalanceViewModel.h"
#include "core/effects/WhiteBalanceEffect.h"
#include "editor/Document.h"
#include "editor/WhiteBalanceUtils.h"

#include <QFutureWatcher>
#include <QtConcurrent>

#include <expected>

WhiteBalanceViewModel::WhiteBalanceViewModel(Document *document, QObject *parent)
    : QObject{parent}
    , m_document{document}
{}

void WhiteBalanceViewModel::pick(QPointF point, double radius)
{
    if (!m_document->isOpen()) {
        return;
    }
    const auto image = m_document->image();
    using Outcome = std::expected<WhiteBalanceUtils::WhiteBalancePick, QString>;
    auto *watcher = new QFutureWatcher<Outcome>{this};
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, image] {
        watcher->deleteLater();
        // Measured on a picture since left: not this one's white balance.
        if (m_document->image() != image) {
            return;
        }
        const Outcome pick = watcher->result();
        if (!pick) {
            Q_EMIT failed(pick.error());
            return;
        }
        // The settings may have changed meanwhile: only the white balance is the pick's.
        core::Settings whiteBalance;
        for (const core::Parameter &parameter :
             {core::WhiteBalanceEffect::TEMPERATURE, core::WhiteBalanceEffect::TINT}) {
            whiteBalance.setValue(parameter.id, pick->settings.value(parameter));
        }
        m_document->mergeSettings(whiteBalance);

        QString message = pick->clamped ? tr("White balance set as near neutral as the sliders go.")
                                        : tr("White balance set from the area picked.");
        if (pick->clipped > 0) {
            message += ' ' + tr("%n clipped pixel(s) left out.", nullptr, pick->clipped);
        }
        Q_EMIT picked(message);
    });
    watcher->setFuture(QtConcurrent::run([image, settings = m_document->settings(), point, radius] {
        return WhiteBalanceUtils::pickWhiteBalance(*image, settings, point, radius);
    }));
}
