// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/ParameterSlider.h"
#include "widgets/SliderStyle.h"

#include <QDoubleSpinBox>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>

#include <algorithm>
#include <cmath>

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
    , m_box{new QDoubleSpinBox{this}}
    , m_value{defaultValue}
    , m_default{defaultValue}
    , m_scale{std::pow(10.0, decimals)}
{
    m_slider->setRange(toSteps(minimum), toSteps(maximum));
    m_slider->setValue(toSteps(defaultValue));
    m_slider->setPageStep(std::max(1, (toSteps(maximum) - toSteps(minimum)) / 20));
    // A line with a circle to drag, whatever the application's style.
    SliderStyle::apply(m_slider);
    m_slider->setToolTip(tr("Double click to reset"));
    m_label->setToolTip(tr("Double click to reset"));

    m_box->setRange(minimum, maximum);
    m_box->setDecimals(decimals);
    m_box->setSingleStep(1.0 / m_scale);
    m_box->setValue(defaultValue);
    m_box->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_box->setAlignment(Qt::AlignRight);
    m_box->setFrame(false);
    m_box->setKeyboardTracking(false);
    m_box->setMaximumWidth(64);

    auto *layout = new QGridLayout{this};
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setVerticalSpacing(0);
    layout->addWidget(m_label, 0, 0);
    layout->addWidget(m_box, 0, 1);
    layout->addWidget(m_slider, 1, 0, 1, 2);

    m_label->installEventFilter(this);
    m_slider->installEventFilter(this);

    connect(m_slider, &QSlider::valueChanged, this, [this](int steps) {
        changeTo(steps / m_scale);
    });
    connect(m_box, &QDoubleSpinBox::valueChanged, this, [this](double value) { changeTo(value); });
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
        && (watched == m_label || watched == m_slider)) {
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
    const QSignalBlocker blockBox{m_box};
    m_slider->setValue(toSteps(value));
    m_box->setValue(value);
}

int ParameterSlider::toSteps(double value) const
{
    return static_cast<int>(std::lround(value * m_scale));
}
