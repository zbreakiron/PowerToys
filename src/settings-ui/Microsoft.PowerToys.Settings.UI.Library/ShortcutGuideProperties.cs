﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Text.Json.Serialization;

namespace Microsoft.PowerToys.Settings.UI.Library
{
    public class ShortcutGuideProperties
    {
        public ShortcutGuideProperties()
        {
            OverlayOpacity = new IntProperty(90);
            PressTime = new IntProperty(900);
            Theme = new StringProperty("system");
            DisabledApps = new StringProperty();
            OpenShortcutGuide = new HotkeySettings(true, false, false, true, 0xBF);
        }

        [JsonPropertyName("open_shortcutguide")]
        public HotkeySettings OpenShortcutGuide { get; set; }

        [JsonPropertyName("overlay_opacity")]
        public IntProperty OverlayOpacity { get; set; }

        [JsonPropertyName("press_time")]
        public IntProperty PressTime { get; set; }

        [JsonPropertyName("theme")]
        public StringProperty Theme { get; set; }

        [JsonPropertyName("disabled_apps")]
        public StringProperty DisabledApps { get; set; }
    }
}
