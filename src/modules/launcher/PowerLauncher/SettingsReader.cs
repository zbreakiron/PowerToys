﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Abstractions;
using System.Linq;
using System.Threading;
using System.Windows.Input;
using Microsoft.PowerToys.Common.UI;
using Microsoft.PowerToys.Settings.UI.Library;
using PowerLauncher.Helper;
using PowerLauncher.Plugin;
using Wox.Infrastructure.Hotkey;
using Wox.Infrastructure.UserSettings;
using Wox.Plugin;
using Wox.Plugin.Logger;
using JsonException = System.Text.Json.JsonException;

namespace PowerLauncher
{
    // Watch for /Local/Microsoft/PowerToys/Launcher/Settings.json changes
    public class SettingsReader : BaseModel
    {
        private readonly ISettingsUtils _settingsUtils;

        private const int MaxRetries = 10;
        private static readonly object _readSyncObject = new object();
        private readonly PowerToysRunSettings _settings;
        private readonly ThemeManager _themeManager;

        private IFileSystemWatcher _watcher;

        public SettingsReader(PowerToysRunSettings settings, ThemeManager themeManager)
        {
            _settingsUtils = new SettingsUtils();
            _settings = settings;
            _themeManager = themeManager;

            // Apply theme at startup
            _themeManager.ChangeTheme(_settings.Theme, true);
        }

        public void CreateSettingsIfNotExists()
        {
            if (!_settingsUtils.SettingsExists(PowerLauncherSettings.ModuleName))
            {
                Log.Info("PT Run settings.json was missing, creating a new one", GetType());

                var defaultSettings = new PowerLauncherSettings();
                defaultSettings.Plugins = GetDefaultPluginsSettings();
                defaultSettings.Save(_settingsUtils);
            }
        }

        public void ReadSettingsOnChange()
        {
            _watcher = Microsoft.PowerToys.Settings.UI.Library.Utilities.Helper.GetFileWatcher(
                PowerLauncherSettings.ModuleName,
                "settings.json",
                () =>
                {
                    Log.Info("Settings were changed. Read settings.", GetType());
                    ReadSettings();
                });
        }

        public void ReadSettings()
        {
            Monitor.Enter(_readSyncObject);
            var retry = true;
            var retryCount = 0;
            while (retry)
            {
                try
                {
                    retryCount++;
                    CreateSettingsIfNotExists();

                    var overloadSettings = _settingsUtils.GetSettingsOrDefault<PowerLauncherSettings>(PowerLauncherSettings.ModuleName);
                    if (overloadSettings != null)
                    {
                        Log.Info($"Successfully read new settings. retryCount={retryCount}", GetType());
                    }

                    if (overloadSettings.Plugins == null || overloadSettings.Plugins.Count() != PluginManager.AllPlugins.Count)
                    {
                        // Needed to be consistent with old settings
                        overloadSettings.Plugins = CombineWithDefaultSettings(overloadSettings.Plugins);
                        _settingsUtils.SaveSettings(overloadSettings.ToJsonString(), PowerLauncherSettings.ModuleName);
                    }
                    else
                    {
                        foreach (var setting in overloadSettings.Plugins)
                        {
                            var plugin = PluginManager.AllPlugins.FirstOrDefault(x => x.Metadata.ID == setting.Id);
                            plugin?.Update(setting, App.API);
                        }
                    }

                    var openPowerlauncher = ConvertHotkey(overloadSettings.Properties.OpenPowerLauncher);
                    if (_settings.Hotkey != openPowerlauncher)
                    {
                        _settings.Hotkey = openPowerlauncher;
                    }

                    if (_settings.MaxResultsToShow != overloadSettings.Properties.MaximumNumberOfResults)
                    {
                        _settings.MaxResultsToShow = overloadSettings.Properties.MaximumNumberOfResults;
                    }

                    if (_settings.IgnoreHotkeysOnFullscreen != overloadSettings.Properties.IgnoreHotkeysInFullscreen)
                    {
                        _settings.IgnoreHotkeysOnFullscreen = overloadSettings.Properties.IgnoreHotkeysInFullscreen;
                    }

                    if (_settings.ClearInputOnLaunch != overloadSettings.Properties.ClearInputOnLaunch)
                    {
                        _settings.ClearInputOnLaunch = overloadSettings.Properties.ClearInputOnLaunch;
                    }

                    if (_settings.Theme != overloadSettings.Properties.Theme)
                    {
                        _settings.Theme = overloadSettings.Properties.Theme;
                        _themeManager.ChangeTheme(_settings.Theme, true);
                    }

                    if (_settings.StartupPosition != overloadSettings.Properties.Position)
                    {
                        _settings.StartupPosition = overloadSettings.Properties.Position;
                    }

                    retry = false;
                }

                // the settings application can hold a lock on the settings.json file which will result in a IOException.
                // This should be changed to properly synch with the settings app instead of retrying.
                catch (IOException e)
                {
                    if (retryCount > MaxRetries)
                    {
                        retry = false;
                        Log.Exception($"Failed to Deserialize PowerToys settings, Retrying {e.Message}", e, GetType());
                    }
                    else
                    {
                        Thread.Sleep(1000);
                    }
                }
                catch (JsonException e)
                {
                    if (retryCount > MaxRetries)
                    {
                        retry = false;
                        Log.Exception($"Failed to Deserialize PowerToys settings, Creating new settings as file could be corrupted {e.Message}", e, GetType());

                        // Settings.json could possibly be corrupted. To mitigate this we delete the
                        // current file and replace it with a correct json value.
                        _settingsUtils.DeleteSettings(PowerLauncherSettings.ModuleName);
                        CreateSettingsIfNotExists();
                        ErrorReporting.ShowMessageBox(Properties.Resources.deseralization_error_title, Properties.Resources.deseralization_error_message);
                    }
                    else
                    {
                        Thread.Sleep(1000);
                    }
                }
            }

            Monitor.Exit(_readSyncObject);
        }

        private static string ConvertHotkey(HotkeySettings hotkey)
        {
            Key key = KeyInterop.KeyFromVirtualKey(hotkey.Code);
            HotkeyModel model = new HotkeyModel(hotkey.Alt, hotkey.Shift, hotkey.Win, hotkey.Ctrl, key);
            return model.ToString();
        }

        private static List<PowerLauncherPluginSettings> CombineWithDefaultSettings(IEnumerable<PowerLauncherPluginSettings> plugins)
        {
            var results = GetDefaultPluginsSettings().ToDictionary(x => x.Id);
            foreach (var plugin in plugins)
            {
                if (results.ContainsKey(plugin.Id))
                {
                    results[plugin.Id] = plugin;
                }
            }

            return results.Values.ToList();
        }

        private static string GetIcon(PluginMetadata metadata, string iconPath)
        {
            var pluginDirectory = Path.GetFileName(metadata.PluginDirectory);
            return Path.Combine(pluginDirectory, iconPath);
        }

        private static IEnumerable<PowerLauncherPluginSettings> GetDefaultPluginsSettings()
        {
            return PluginManager.AllPlugins.Select(x => new PowerLauncherPluginSettings()
            {
                Id = x.Metadata.ID,
                Name = x.Plugin == null ? x.Metadata.Name : x.Plugin.Name,
                Description = x.Plugin?.Description,
                Author = x.Metadata.Author,
                Disabled = x.Metadata.Disabled,
                IsGlobal = x.Metadata.IsGlobal,
                ActionKeyword = x.Metadata.ActionKeyword,
                IconPathDark = GetIcon(x.Metadata, x.Metadata.IcoPathDark),
                IconPathLight = GetIcon(x.Metadata, x.Metadata.IcoPathLight),
                AdditionalOptions = x.Plugin is ISettingProvider ? (x.Plugin as ISettingProvider).AdditionalOptions : new List<PluginAdditionalOption>(),
            });
        }
    }
}
