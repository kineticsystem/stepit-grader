// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/ParameterSlider.h"
#include "widgets/SliderStyle.h"

#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QLocale>
#include <QSignalBlocker>
#include <QSlider>

#include <algorithm>
#include <cmath>
#include <initializer_list>

ParameterSlider::ParameterSlider(
    const QString &label,
    double minimum,
    double maximum,
    double defaultValue,
    int decimals,
    QWidget *parent)
    : QWidget{parent}
    , m_label{new QLabel{label, this}}
    , m_slider{new QSlider{Qt::Horizontal, this}}
    , m_valueLabel{new QLabel{this}}
    , m_value{defaultValue}
    , m_default{defaultValue}
    , m_decimals{decimals}
    , m_scale{std::pow(10.0, decimals)}
{
    m_slider->setRange(toSteps(minimum), toSteps(maximum));
    m_slider->setValue(toSteps(defaultValue));
    m_slider->setPageStep(std::max(1, (toSteps(maximum) - toSteps(minimum)) / 20));
    // A line with a circle to drag, whatever the application's style.
    SliderStyle::apply(m_slider);
    for (QWidget *resets : std::initializer_list<QWidget *>{m_slider, m_label, m_valueLabel}) {
        resets->setToolTip(tr("Double click to reset"));
    }
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    show(defaultValue);

    auto *layout = new QGridLayout{this};
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setVerticalSpacing(0);
    layout->addWidget(m_label, 0, 0);
    layout->addWidget(m_valueLabel, 0, 1);
    layout->addWidget(m_slider, 1, 0, 1, 2);

    m_label->installEventFilter(this);
    m_valueLabel->installEventFilter(this);
    m_slider->installEventFilter(this);

    connect(m_slider, &QSlider::valueChanged, this, [this](int steps) {
        changeTo(steps / m_scale);
    });
}

void ParameterSlider::setValue(double value)
{
    m_value = value;
    show(value);
}

void ParameterSlider::reset()
{
    show(m_default);
    changeTo(m_default);
}

bool ParameterSlider::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick
        && (watched == m_label || watched == m_valueLabel || watched == m_slider)) {
        reset();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ParameterSlider::changeTo(double value)
{
    if (value == m_value) {
        return;
    }
    m_value = value;
    show(value);
    Q_EMIT valueChanged(value);
}

void ParameterSlider::show(double value)
{
    const QSignalBlocker blockSlider{m_slider};
    m_slider->setValue(toSteps(value));
    // As the slider's step: no "-0.00" for a value a hair below 0.
    const double shown = toSteps(value) / m_scale;
    m_valueLabel->setText(QLocale{}.toString(shown == 0 ? 0.0 : shown, 'f', m_decimals));
}

int ParameterSlider::toSteps(double value) const
{
    return static_cast<int>(std::lround(value * m_scale));
}
