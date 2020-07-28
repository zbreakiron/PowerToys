using ColorPicker.Controls;
using ColorPicker.Settings;
using System;
using System.ComponentModel.Composition;
using System.Linq;
using System.Windows;
using System.Windows.Threading;

namespace ColorPicker.Helpers
{
    [Export(typeof(AppStateHandler))]
    public class AppStateHandler
    {
        private Dispatcher _dispatcher;
        private readonly IUserSettings _userSettings;

        [ImportingConstructor]
        public AppStateHandler(IUserSettings userSettings)
        {
            _dispatcher = Application.Current.MainWindow.Dispatcher;
            _userSettings = userSettings;
            Application.Current.MainWindow.Closed += MainWindow_Closed;
        }

        public event EventHandler AppShown;

        public event EventHandler AppHidden;

        public event EventHandler AppClosed;

        public void ShowColorPicker()
        {
            AppShown?.Invoke(this, EventArgs.Empty);
            Application.Current.MainWindow.Opacity = 0;
            Application.Current.MainWindow.Visibility = Visibility.Visible;
        }

        public void HideColorPicker()
        {
            Application.Current.MainWindow.Opacity = 0;
            Application.Current.MainWindow.Visibility = Visibility.Collapsed;
            AppHidden?.Invoke(this, EventArgs.Empty);
        }

        public void ShowEditor()
        {
            _dispatcher.Invoke(() =>
            {
                ColorPickerEditorWindow editor = new ColorPickerEditorWindow(_userSettings, this);
                editor.Show();
            });
        }

        public static void SetTopMost()
        {
            Application.Current.MainWindow.Topmost = false;
            Application.Current.MainWindow.Topmost = true;
        }

        private void MainWindow_Closed(object sender, EventArgs e)
        {
            AppClosed?.Invoke(this, EventArgs.Empty);
        }
    }
}
