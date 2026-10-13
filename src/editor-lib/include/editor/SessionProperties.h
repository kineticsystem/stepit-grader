// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

/**
 * The keys of what the editor keeps between sessions in `QSettings`, all in one place, so no two
 * parts use the same key for different things. A key is never renamed: the value saved under the
 * old one would be lost.
 */
namespace SessionProperties {

/// Where the dialogs start, and the pictures opened last.
namespace Files {

/// The folder a picture was last opened from.
inline constexpr const char *LAST_FOLDER = "lastFolder";
/// The folder settings were last loaded from: often not where the pictures are.
inline constexpr const char *LAST_SETTINGS_FOLDER = "lastSettingsFolder";
/// The pictures of File > Open Recent, the latest first.
inline constexpr const char *RECENT_FILES = "recentPictures";

} // namespace Files

/// The user's choices.
namespace Options {

/// The format the picture was last saved as, by its filter: the next save offers it first.
inline constexpr const char *IMAGE_FORMAT = "imageFormat";
/// The theme's name, e.g. "dark", applied when the editor starts.
inline constexpr const char *THEME = "theme";

} // namespace Options

} // namespace SessionProperties
