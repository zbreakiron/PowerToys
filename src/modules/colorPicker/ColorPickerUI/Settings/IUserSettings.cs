using Microsoft.PowerToys.Settings.UI.Lib;
using System.Collections.ObjectModel;

namespace ColorPicker.Settings
{
    public interface IUserSettings
    {
        SettingItem<string> ActivationShortcut { get; }

        SettingItem<bool> ChangeCursor { get; }

        SettingItem<ColorRepresentationType> CopiedColorRepresentation { get; }

        SettingItem<bool> OpenEditor { get; }

        ObservableCollection<string> ColorHistory { get; }
    }
}
