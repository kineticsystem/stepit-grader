// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

class QDoubleSpinBox;
class QLabel;
class QSlider;

/**
 * A named value set with a slider or typed in, e.g. "Exposure +0.50". A double click on the name
 * or the slider puts the default back. It knows nothing of what the value does.
 */
class ParameterSlider : public QWidget
{
    Q_OBJECT

public:
    ParameterSlider(
        const QString &label,
        double minimum,
        double maximum,
        double defaultValue,
        int decimals,
        QWidget *parent = nullptr);

    double value() const { return m_value; }
    double defaultValue() const { return m_default; }

    /// Shows the value, without emitting valueChanged().
    void setValue(double value);

public Q_SLOTS:
    /// Puts the default back, as the user would.
    void reset();

Q_SIGNALS:
    /// The user changed the value: with the slider, the box or a reset. Not sent by setValue().
    void valueChanged(double value);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void changeTo(double value);
    void show(double value);
    int toSteps(double value) const;

    QLabel *m_label;
    QSlider *m_slider;
    QDoubleSpinBox *m_box;
    double m_value;
    double m_default;
    double m_scale;
};
