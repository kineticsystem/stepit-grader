// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "imageio/XmpSidecar.h"
#include "imageio/Error.h"

#include <pugixml.hpp>

#include <charconv>
#include <cmath>
#include <format>
#include <fstream>
#include <sstream>

namespace imageio {

namespace {

constexpr const char *RDF_NAMESPACE = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";

/// What Lightroom needs to read the settings with the 2012 meaning of the tone controls.
constexpr const char *PROCESS_VERSION = "11.0";

constexpr unsigned PARSE_OPTIONS = pugi::parse_default | pugi::parse_declaration | pugi::parse_pi
                                   | pugi::parse_comments;

std::string localName(const char *qualified)
{
    const std::string name{qualified};
    const auto colon = name.find(':');
    return colon == std::string::npos ? name : name.substr(colon + 1);
}

/// The prefix bound to the namespace where `node` is, or an empty string.
std::string prefixOf(pugi::xml_node node, const std::string &uri)
{
    for (; node; node = node.parent()) {
        for (const pugi::xml_attribute attribute : node.attributes()) {
            const std::string name = attribute.name();
            if (name.starts_with("xmlns:") && uri == attribute.value()) {
                return name.substr(6);
            }
        }
    }
    return {};
}

/// Every rdf:Description, whatever its prefix.
std::vector<pugi::xml_node> descriptions(const pugi::xml_document &document)
{
    std::vector<pugi::xml_node> found;
    for (const pugi::xpath_node &node : document.select_nodes("//*")) {
        const pugi::xml_node element = node.node();
        if (localName(element.name()) == "Description"
            && prefixOf(element, RDF_NAMESPACE) + ":Description" == element.name()) {
            found.push_back(element);
        }
    }
    return found;
}

std::optional<double> number(std::string text)
{
    if (!text.empty() && text.front() == '+') {
        text.erase(0, 1);
    }
    double value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream in{path, std::ios::binary};
    if (!in) {
        throw Error{"Cannot read " + path.string()};
    }
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

pugi::xml_node newDocument(pugi::xml_document &document)
{
    pugi::xml_node meta = document.append_child("x:xmpmeta");
    meta.append_attribute("xmlns:x") = "adobe:ns:meta/";
    meta.append_attribute("x:xmptk") = "stepit-grader";
    pugi::xml_node rdf = meta.append_child("rdf:RDF");
    rdf.append_attribute("xmlns:rdf") = RDF_NAMESPACE;
    pugi::xml_node description = rdf.append_child("rdf:Description");
    description.append_attribute("rdf:about") = "";
    return description;
}

void setAttribute(pugi::xml_node node, const std::string &name, const std::string &value)
{
    pugi::xml_attribute attribute = node.attribute(name.c_str());
    if (!attribute) {
        attribute = node.append_attribute(name.c_str());
    }
    attribute.set_value(value.c_str());
    // The same setting as an element would contradict the attribute.
    while (pugi::xml_node element = node.child(name.c_str())) {
        node.remove_child(element);
    }
}

} // namespace

namespace xmp {

std::string format(const core::Parameter &parameter, double value)
{
    const double rounded = std::round(value * std::pow(10.0, parameter.decimals))
                           / std::pow(10.0, parameter.decimals);
    if (parameter.minimum < 0 && rounded > 0) {
        return std::format("+{:.{}f}", rounded, parameter.decimals);
    }
    return std::format("{:.{}f}", rounded == 0 ? 0.0 : rounded, parameter.decimals);
}

core::Settings parse(const std::string &document, const std::vector<core::Parameter> &parameters)
{
    pugi::xml_document xml;
    if (!xml.load_string(document.c_str(), PARSE_OPTIONS)) {
        throw Error{"not an XMP document"};
    }
    core::Settings settings;
    for (const pugi::xml_node description : descriptions(xml)) {
        const std::string prefix = prefixOf(description, CRS_NAMESPACE);
        if (prefix.empty()) {
            continue;
        }
        for (const core::Parameter &parameter : parameters) {
            const std::string name = prefix + ":" + parameter.id;
            std::optional<double> value;
            if (const pugi::xml_attribute attribute = description.attribute(name.c_str())) {
                value = number(attribute.value());
            } else if (const pugi::xml_node element = description.child(name.c_str())) {
                value = number(element.text().get());
            }
            if (value) {
                settings.setValue(parameter.id, *value);
            }
        }
    }
    return settings;
}

std::string write(
    const std::string &document,
    const core::Settings &settings,
    const std::vector<core::Parameter> &parameters)
{
    pugi::xml_document xml;
    pugi::xml_node target;
    if (document.empty()) {
        target = newDocument(xml);
    } else {
        if (!xml.load_string(document.c_str(), PARSE_OPTIONS)) {
            throw Error{"not an XMP document"};
        }
        const auto found = descriptions(xml);
        if (found.empty()) {
            throw Error{"an XMP document without rdf:Description"};
        }
        target = found.front();
        for (const pugi::xml_node description : found) {
            if (!prefixOf(description, CRS_NAMESPACE).empty()) {
                target = description;
                break;
            }
        }
    }

    std::string prefix = prefixOf(target, CRS_NAMESPACE);
    if (prefix.empty()) {
        prefix = "crs";
        target.append_attribute("xmlns:crs") = CRS_NAMESPACE;
    }
    if (!target.attribute((prefix + ":ProcessVersion").c_str())) {
        setAttribute(target, prefix + ":ProcessVersion", PROCESS_VERSION);
    }
    setAttribute(target, prefix + ":HasSettings", "True");
    for (const core::Parameter &parameter : parameters) {
        setAttribute(
            target, prefix + ":" + parameter.id, format(parameter, settings.value(parameter)));
    }

    std::ostringstream out;
    xml.save(out, " ", pugi::format_default | pugi::format_no_declaration);
    return out.str();
}

} // namespace xmp

core::Settings XmpSidecar::loadFile(
    const std::filesystem::path &xmp, const std::vector<core::Parameter> &parameters) const
{
    if (!std::filesystem::exists(xmp)) {
        throw Error{"Cannot read " + xmp.string() + ": no such file"};
    }
    try {
        return xmp::parse(readFile(xmp), parameters);
    } catch (const Error &e) {
        throw Error{"Cannot read " + xmp.string() + ": " + e.what()};
    }
}

std::filesystem::path XmpSidecar::location(const std::filesystem::path &image) const
{
    std::filesystem::path sidecar = image;
    return sidecar.replace_extension(".xmp");
}

std::optional<core::Settings> XmpSidecar::load(
    const std::filesystem::path &image, const std::vector<core::Parameter> &parameters) const
{
    const std::filesystem::path path = location(image);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }
    try {
        return xmp::parse(readFile(path), parameters);
    } catch (const Error &e) {
        throw Error{"Cannot read " + path.string() + ": " + e.what()};
    }
}

void XmpSidecar::save(
    const std::filesystem::path &image,
    const core::Settings &settings,
    const std::vector<core::Parameter> &parameters) const
{
    const std::filesystem::path path = location(image);
    std::string text;
    try {
        const std::string existing = std::filesystem::exists(path) ? readFile(path) : std::string{};
        text = xmp::write(existing, settings, parameters);
    } catch (const Error &e) {
        throw Error{"Cannot update " + path.string() + ": " + e.what()};
    }

    // Written whole to a temporary file, then renamed over the sidecar, so a failure never leaves
    // half a file.
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream out{temporary, std::ios::binary | std::ios::trunc};
        out << text;
        if (!out) {
            throw Error{"Cannot write " + temporary.string()};
        }
    }
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(temporary, error);
        throw Error{"Cannot write " + path.string()};
    }
}

} // namespace imageio
