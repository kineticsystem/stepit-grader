// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/AdjustmentsPanel.h"
#include "widgets/CollapsibleSection.h"
#include "widgets/ParameterSlider.h"

#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

/// The space above a heading that follows other sliders, in pixels, so its group stands apart.
constexpr int HEADING_SPACE = 12;
/// The space between a heading and the first slider of its group, in pixels.
constexpr int HEADING_GAP = 4;

} // namespace

AdjustmentsPanel::AdjustmentsPanel(std::vector<Section> sections, QWidget *parent)
    : QWidget{parent}
{
    auto *content = new QWidget;
    auto *column = new QVBoxLayout{content};
    column->setContentsMargins(12, 8, 12, 8);
    column->setSpacing(8);
    for (const Section &section : sections) {
        auto *group = new CollapsibleSection{section.title};
        column->addWidget(group);
        m_sections[section.title] = group;
        connect(
            group,
            &CollapsibleSection::expandedChanged,
            this,
            [this, title = section.title](bool expanded) {
                Q_EMIT sectionExpandedChanged(title, expanded);
            });

        for (const core::Parameter &parameter : section.parameters) {
            auto *slider = new ParameterSlider{
                QString::fromStdString(parameter.label),
                parameter.minimum,
                parameter.maximum,
                parameter.defaultValue,
                parameter.decimals};
            group->addWidget(slider);
            m_sliders[parameter.id] = slider;
            m_parameters.push_back(parameter);
            connect(
                slider,
                &ParameterSlider::valueChanged,
                this,
                [this, id = parameter.id](double value) {
                    m_settings.setValue(id, value);
                    Q_EMIT settingsChanged(m_settings);
                });
        }
    }
    column->addStretch();

    auto *scroll = new QScrollArea{this};
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);
}

void AdjustmentsPanel::setSettings(const core::Settings &settings)
{
    m_settings = settings;
    for (const core::Parameter &parameter : m_parameters) {
        m_sliders.at(parameter.id)->setValue(settings.value(parameter));
    }
}

ParameterSlider *AdjustmentsPanel::slider(const std::string &id) const
{
    const auto it = m_sliders.find(id);
    return it == m_sliders.end() ? nullptr : it->second;
}

QStringList AdjustmentsPanel::sectionTitles() const
{
    QStringList titles;
    for (const auto &[title, section] : m_sections) {
        titles << title;
    }
    return titles;
}

bool AdjustmentsPanel::isSectionExpanded(const QString &title) const
{
    const auto it = m_sections.find(title);
    return it != m_sections.end() && it->second->isExpanded();
}

void AdjustmentsPanel::setSectionExpanded(const QString &title, bool expanded)
{
    const auto it = m_sections.find(title);
    if (it != m_sections.end()) {
        it->second->setExpanded(expanded);
    }
}

void AdjustmentsPanel::addSectionHeaderWidget(const QString &title, QWidget *widget)
{
    const auto it = m_sections.find(title);
    if (it != m_sections.end()) {
        it->second->addHeaderWidget(widget);
    }
}

void AdjustmentsPanel::addSectionWidget(const QString &title, QWidget *widget)
{
    const auto it = m_sections.find(title);
    if (it != m_sections.end()) {
        it->second->addWidgetFirst(widget);
    }
}

void AdjustmentsPanel::setControlsEnabled(bool enabled)
{
    for (const auto &[id, slider] : m_sliders) {
        slider->setEnabled(enabled);
    }
}

void AdjustmentsPanel::addHeading(const std::string &id, const QString &text)
{
    const auto slider = m_sliders.find(id);
    if (slider == m_sliders.end()) {
        return;
    }
    auto *heading = new QLabel{text};
    QFont font = heading->font();
    font.setBold(true);
    heading->setFont(font);
    auto *box = new QWidget;
    auto *layout = new QVBoxLayout{box};
    // The first slider of a section has the title's space above it already.
    const bool first = slider->second->parentWidget()->layout()->indexOf(slider->second) == 0;
    layout->setContentsMargins(0, first ? 0 : HEADING_SPACE, 0, HEADING_GAP);
    layout->addWidget(heading);
    for (const auto &[title, section] : m_sections) {
        if (section->addWidgetBefore(slider->second, box)) {
            return;
        }
    }
    delete box;
}
