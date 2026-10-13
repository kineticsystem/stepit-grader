// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/CollapsibleSection.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

/// The space between the title and what the section holds, in pixels, so the first slider does
/// not crowd the title.
constexpr int CONTENT_TOP_MARGIN = 8;

} // namespace

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *parent)
    : QWidget{parent}
    , m_header{new QToolButton{this}}
    , m_headerLayout{new QHBoxLayout}
    , m_content{new QWidget{this}}
    , m_contentLayout{new QVBoxLayout{m_content}}
{
    m_header->setText(title);
    // Always checked, open or closed, so the style draws the title as a block either way and the
    // sections stand apart; whether it is open is m_expanded.
    m_header->setCheckable(true);
    m_header->setChecked(true);
    m_header->setArrowType(Qt::DownArrow);
    m_header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_header->setAutoRaise(true);
    m_header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont font = m_header->font();
    font.setBold(true);
    m_header->setFont(font);

    m_contentLayout->setContentsMargins(0, CONTENT_TOP_MARGIN, 0, 0);
    m_contentLayout->setSpacing(0);

    auto *layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(2);
    m_headerLayout->addWidget(m_header);
    layout->addLayout(m_headerLayout);
    layout->addWidget(m_content);

    connect(m_header, &QToolButton::clicked, this, [this] {
        m_header->setChecked(true);
        showContent(!m_expanded);
        Q_EMIT expandedChanged(m_expanded);
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

void CollapsibleSection::addWidgetFirst(QWidget *widget)
{
    m_contentLayout->insertWidget(0, widget);
}

bool CollapsibleSection::addWidgetBefore(QWidget *existing, QWidget *widget)
{
    const int index = m_contentLayout->indexOf(existing);
    if (index < 0) {
        return false;
    }
    m_contentLayout->insertWidget(index, widget);
    return true;
}

void CollapsibleSection::addHeaderWidget(QWidget *widget)
{
    m_headerLayout->addWidget(widget);
}

bool CollapsibleSection::isExpanded() const
{
    return m_expanded;
}

void CollapsibleSection::setExpanded(bool expanded)
{
    showContent(expanded);
}

void CollapsibleSection::showContent(bool expanded)
{
    m_expanded = expanded;
    m_header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    m_content->setVisible(expanded);
}
