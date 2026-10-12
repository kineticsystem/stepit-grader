// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/widgets/AdjustmentsPanel.h"
#include "editor/widgets/CollapsibleSection.h"
#include "editor/widgets/ParameterSlider.h"

#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>

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

void AdjustmentsPanel::setControlsEnabled(bool enabled)
{
    for (const auto &[id, slider] : m_sliders) {
        slider->setEnabled(enabled);
    }
}
