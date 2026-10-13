// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Parameter.h"
#include "core/Settings.h"

#include <QString>
#include <QStringList>
#include <QWidget>

#include <map>
#include <string>
#include <vector>

class CollapsibleSection;
class ParameterSlider;

/**
 * The tools: one slider per parameter, in sections that fold away, e.g. "Light" with the exposure
 * and the contrast. It knows the parameters it is given, not the effects behind them, and tells
 * its client of every change with settingsChanged(), and of every section opened or closed with
 * sectionExpandedChanged(), so the client can remember them.
 */
class AdjustmentsPanel : public QWidget
{
    Q_OBJECT

public:
    struct Section
    {
        QString title;
        std::vector<core::Parameter> parameters;
    };

    explicit AdjustmentsPanel(std::vector<Section> sections, QWidget *parent = nullptr);

    core::Settings settings() const { return m_settings; }

    /// Shows the settings, without emitting settingsChanged().
    void setSettings(const core::Settings &settings);

    ParameterSlider *slider(const std::string &id) const;

    /**
     * Enables or disables the sliders only: the sections still fold and the panel still scrolls.
     * Disabling the whole panel would disable its titles and its scroll bar too.
     */
    void setControlsEnabled(bool enabled);

    QStringList sectionTitles() const;
    bool isSectionExpanded(const QString &title) const;
    /// Opens or closes the section, without emitting sectionExpandedChanged().
    void setSectionExpanded(const QString &title, bool expanded);
    /// Adds a widget, e.g. a tool's button, on the right of the section's title.
    void addSectionHeaderWidget(const QString &title, QWidget *widget);
    /// Adds a widget above the section's sliders, e.g. a drawing of what they set.
    void addSectionWidget(const QString &title, QWidget *widget);
    /**
     * Adds a heading just above the parameter's slider, to name the group of sliders it starts,
     * e.g. "Hue" above the hues of the colour mixer. A heading after other sliders stands apart
     * from them.
     */
    void addHeading(const std::string &id, const QString &text);

Q_SIGNALS:
    /// The user changed a value; `settings` has every value.
    void settingsChanged(const core::Settings &settings);
    /// The user opened or closed a section.
    void sectionExpandedChanged(const QString &title, bool expanded);

private:
    std::map<QString, CollapsibleSection *> m_sections;
    std::map<std::string, ParameterSlider *> m_sliders;
    std::vector<core::Parameter> m_parameters;
    core::Settings m_settings;
};
