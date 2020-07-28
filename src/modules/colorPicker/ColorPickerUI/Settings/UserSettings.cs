using System.IO;
using System.ComponentModel.Composition;
using Microsoft.PowerToys.Settings.UI.Lib;
using Microsoft.PowerToys.Settings.UI.Lib.Utilities;
using ColorPicker.Helpers;
using System.Threading.Tasks;
using System.Threading;
using System;
using System.Collections.ObjectModel;
using System.Linq;

namespace ColorPicker.Settings
{
    [Export(typeof(IUserSettings))]
    public class UserSettings : IUserSettings
    {
        private const string ColorPickerModuleName = "ColorPicker";
        private const string DefaultActivationShortcut = "Ctrl + Break";
        private const int MaxNumberOfRetry = 5;
        private FileSystemWatcher _watcher;
        private bool _loadingColors;

        private object _loadingSettingsLock = new object();

        [ImportingConstructor]
        public UserSettings()
        {
            ChangeCursor = new SettingItem<bool>(true);
            ActivationShortcut = new SettingItem<string>(DefaultActivationShortcut);
            CopiedColorRepresentation = new SettingItem<ColorRepresentationType>(ColorRepresentationType.HEX);
            OpenEditor = new SettingItem<bool>(false);
            ColorHistory = new ObservableCollection<string>();
            ColorHistory.CollectionChanged += ColorHistory_CollectionChanged;
            LoadSettingsFromJson();
            _watcher = Helper.GetFileWatcher(ColorPickerModuleName, "settings.json", LoadSettingsFromJson);
        }

        public SettingItem<string> ActivationShortcut { get; private set; }

        public SettingItem<bool> ChangeCursor { get; private set; }

        public SettingItem<ColorRepresentationType> CopiedColorRepresentation { get; private set; }

        public SettingItem<bool> OpenEditor { get; private set; }

        public ObservableCollection<string> ColorHistory { get; private set; }

        private void ColorHistory_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            if (!_loadingColors)
            {
                var settings = SettingsUtils.GetSettings<ColorPickerSettings>(ColorPickerModuleName);
                settings.Properties.ColorHistory = ColorHistory.ToList();
                settings.Save();
            }
        }

        private void LoadSettingsFromJson()
        {
            // TODO this IO call should by Async, update GetFileWatcher helper to support async
            lock (_loadingSettingsLock)
            {
                {
                    var retry = true;
                    var retryCount = 0;

                    while (retry)
                    {
                        try
                        {
                            retryCount++;

                            if (!SettingsUtils.SettingsExists(ColorPickerModuleName))
                            {
                                Logger.LogInfo("ColorPicker settings.json was missing, creating a new one");
                                var defaultColorPickerSettings = new ColorPickerSettings();
                                defaultColorPickerSettings.Save();
                            }
                            var settings = SettingsUtils.GetSettings<ColorPickerSettings>(ColorPickerModuleName);
                            if (settings != null)
                            {
                                ChangeCursor.Value = settings.Properties.ChangeCursor;
                                ActivationShortcut.Value = settings.Properties.ActivationShortcut.ToString();
                                CopiedColorRepresentation.Value = settings.Properties.CopiedColorRepresentation;
                                OpenEditor.Value = settings.Properties.OpenEditor;

                                if(settings.Properties.ColorHistory == null)
                                {
                                    settings.Properties.ColorHistory = new System.Collections.Generic.List<string>();
                                }
                                _loadingColors = true;
                                ColorHistory.Clear();
                                foreach(var item in settings.Properties.ColorHistory)
                                {
                                    ColorHistory.Add(item);
                                }

                                _loadingColors = false;
                            }

                            retry = false;
                        }
                        catch (IOException ex)
                        {
                            if (retryCount > MaxNumberOfRetry)
                            {
                                retry = false;
                            }
                            Logger.LogError("Failed to read changed settings", ex);
                            Thread.Sleep(500);
                        }
                        catch (Exception ex)
                        {
                            Logger.LogError("Failed to read changed settings", ex);
                        }
                    }
                };
            }
        }
    }
}
