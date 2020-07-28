using ColorPicker.Helpers;
using ColorPicker.Settings;
using Microsoft.Toolkit.Wpf.UI.XamlHost;
using System;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;
using Windows.UI;

namespace ColorPicker.Controls
{
    /// <summary>
    /// Interaction logic for ColorPickerEditorWindow.xaml
    /// </summary>
    public partial class ColorPickerEditorWindow : Window
    {
        private readonly IUserSettings _userSettings;
        private readonly AppStateHandler _appStateHandler;
        private WindowsXamlHost _windowsXamlHost;
        private ColorPickerEditorUI.ColorPickerEditor _editorControl;
        private Dispatcher _dispatcher;

        public ColorPickerEditorWindow(IUserSettings userSettings, AppStateHandler appStateHandler)
        {
            _userSettings = userSettings;
            _appStateHandler = appStateHandler;
            _dispatcher = Application.Current.MainWindow.Dispatcher;

            InitializeComponent();
        }

        private void colorPickerEditor_ChildChanged(object sender, EventArgs e)
        {
            if (_windowsXamlHost == null)
            {
                _windowsXamlHost =
                sender as WindowsXamlHost;
            }

            _editorControl = _windowsXamlHost.GetUwpInternalObject() as ColorPickerEditorUI.ColorPickerEditor;

            if (_editorControl != null)
            {
                InitializeColors(_editorControl, _userSettings);
                _editorControl.PickedColors.CollectionChanged += PickedColors_CollectionChanged;
                _editorControl.OpenColorPickerClicked += EditorControl_OpenColorPickerClicked;
                _editorControl.OpenColorPickerPopupClicked += UserControl_OpenColorPickerPopupClicked;
                _editorControl.CloseApplicationClicked += UserControl_CloseApplicationClicked;
                _editorControl.ColorCopied += EditorControl_ColorCopied;
            }
        }

        private void EditorControl_OpenColorPickerClicked(object sender, EventArgs e)
        {
            _appStateHandler.ShowColorPicker();
            CloseWindow();
        }

        private void EditorControl_ColorCopied(object sender, string copiedColor)
        {
            if (!string.IsNullOrEmpty(copiedColor))
            {
                ClipboardHelper.CopyToClipboard(copiedColor);
            }
        }

        private void PickedColors_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            _userSettings.ColorHistory.Clear();
            foreach(var item in _editorControl.PickedColors)
            {
                _userSettings.ColorHistory.Add(item.A + "|" + item.R + "|" + item.G + "|" + item.B);
            }
        }

        private void InitializeColors(ColorPickerEditorUI.ColorPickerEditor editor, IUserSettings userSettings)
        {
            foreach (var item in userSettings.ColorHistory)
            {
                var parts = item.Split('|');
                editor.PickedColors.Add(new Color()
                {
                    A = byte.Parse(parts[0]),
                    R = byte.Parse(parts[1]),
                    G = byte.Parse(parts[2]),
                    B = byte.Parse(parts[3])
                });
            }
            editor.SelectFirstColor();
        }

        private void UserControl_CloseApplicationClicked(object sender, EventArgs e)
        {
            CloseWindow();
        }

        private void UserControl_OpenColorPickerPopupClicked(object sender, Color initialColor)
        {
            Task.Run(async () =>
            {
                await Task.Delay(10);
                _dispatcher.Invoke(() =>
                {
                    var window = new ColorPickerEditorPopup(initialColor);
                    window.Owner = this;
                    if (window.ShowDialog() == true)
                    {
                        _editorControl.AddColor(window.SelectedColor);
                    }
                });
            });
        }

        private void CloseWindow()
        {
            _editorControl.PickedColors.CollectionChanged -= PickedColors_CollectionChanged;
            _editorControl.OpenColorPickerPopupClicked -= UserControl_OpenColorPickerPopupClicked;
            _editorControl.CloseApplicationClicked -= UserControl_CloseApplicationClicked;
            _editorControl.ColorCopied -= EditorControl_ColorCopied;
            _editorControl.OpenColorPickerClicked -= EditorControl_OpenColorPickerClicked;
            _windowsXamlHost.Dispose();
            Close();
        }
    }
}
