// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

class QHBoxLayout;
class QToolButton;
class QVBoxLayout;

/**
 * A titled group of widgets that folds away: a click on the title, or its arrow, shows or hides
 * the widgets below it. It knows nothing of what it holds.
 */
class CollapsibleSection : public QWidget
{
    Q_OBJECT

public:
    explicit CollapsibleSection(const QString &title, QWidget *parent = nullptr);

    QString title() const;

    /// Adds a widget below the title, after those added before.
    void addWidget(QWidget *widget);
    /// Adds a widget below the title, before those added already.
    void addWidgetFirst(QWidget *widget);
    /// Adds a widget just above `existing`, if the section holds it; says whether it does.
    bool addWidgetBefore(QWidget *existing, QWidget *widget);
    /// Adds a widget, e.g. a small button, on the right of the title, always shown.
    void addHeaderWidget(QWidget *widget);

    bool isExpanded() const;

public Q_SLOTS:
    /// Shows or hides the widgets, without emitting expandedChanged().
    void setExpanded(bool expanded);

Q_SIGNALS:
    /// The user opened or closed the section.
    void expandedChanged(bool expanded);

private:
    void showContent(bool expanded);

    QToolButton *m_header;
    QHBoxLayout *m_headerLayout;
    QWidget *m_content;
    bool m_expanded = true;
    QVBoxLayout *m_contentLayout;
};
