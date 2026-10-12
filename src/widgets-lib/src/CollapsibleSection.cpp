// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/CollapsibleSection.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *parent)
    : QWidget{parent}
    , m_header{new QToolButton{this}}
    , m_headerLayout{new QHBoxLayout}
    , m_content{new QWidget{this}}
    , m_contentLayout{new QVBoxLayout{m_content}}
{
    m_header->setText(title);
    m_header->setCheckable(true);
    m_header->setChecked(true);
    m_header->setArrowType(Qt::DownArrow);
    m_header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_header->setAutoRaise(true);
    m_header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont font = m_header->font();
    font.setBold(true);
    m_header->setFont(font);

    auto *line = new QFrame{this};
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(0);

    auto *layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(2);
    m_headerLayout->addWidget(m_header);
    layout->addLayout(m_headerLayout);
    layout->addWidget(line);
    layout->addWidget(m_content);

    connect(m_header, &QToolButton::toggled, this, [this](bool expanded) {
        showContent(expanded);
        Q_EMIT expandedChanged(expanded);
    });
}

QString CollapsibleSection::title() const
{
    return m_header->text();
}

void CollapsibleSection::addWidget(QWidget *widget)
{
    m_contentLayout->addWidget(widget);
}

void CollapsibleSection::addHeaderWidget(QWidget *widget)
{
    m_headerLayout->addWidget(widget);
}

bool CollapsibleSection::isExpanded() const
{
    return m_header->isChecked();
}

void CollapsibleSection::setExpanded(bool expanded)
{
    const QSignalBlocker blocker{m_header};
    m_header->setChecked(expanded);
    showContent(expanded);
}

void CollapsibleSection::showContent(bool expanded)
{
    m_header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    m_content->setVisible(expanded);
}
