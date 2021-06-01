﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.ComponentModel;
using System.Linq;
using System.Timers;
using System.Windows;
using System.Windows.Automation.Peers;
using System.Windows.Controls;
using System.Windows.Input;
using interop;
using Microsoft.PowerLauncher.Telemetry;
using Microsoft.PowerToys.Telemetry;
using PowerLauncher.Helper;
using PowerLauncher.Plugin;
using PowerLauncher.Telemetry.Events;
using PowerLauncher.ViewModel;
using Wox.Infrastructure.UserSettings;
using KeyEventArgs = System.Windows.Input.KeyEventArgs;
using Log = Wox.Plugin.Logger.Log;
using Screen = System.Windows.Forms.Screen;

namespace PowerLauncher
{
    public partial class MainWindow : IDisposable
    {
        private readonly PowerToysRunSettings _settings;
        private readonly MainViewModel _viewModel;
        private bool _isTextSetProgrammatically;
        private bool _deletePressed;
        private Timer _firstDeleteTimer = new Timer();
        private bool _coldStateHotkeyPressed;

        public MainWindow(PowerToysRunSettings settings, MainViewModel mainVM)
            : this()
        {
            DataContext = mainVM;
            _viewModel = mainVM;
            _settings = settings;

            InitializeComponent();

            _firstDeleteTimer.Elapsed += CheckForFirstDelete;
            _firstDeleteTimer.Interval = 1000;
            NativeEventWaiter.WaitForEventLoop(Constants.RunSendSettingsTelemetryEvent(), SendSettingsTelemetry);
        }

        private void SendSettingsTelemetry()
        {
            Log.Info("Send Run settings telemetry", this.GetType());
            var plugins = PluginManager.AllPlugins.ToDictionary(x => x.Metadata.Name, x => new PluginModel()
            {
                Disabled = x.Metadata.Disabled,
                ActionKeyword = x.Metadata.ActionKeyword,
                IsGlobal = x.Metadata.IsGlobal,
            });

            var telemetryEvent = new RunPluginsSettingsEvent(plugins);
            PowerToysTelemetry.Log.WriteEvent(telemetryEvent);
        }

        private void CheckForFirstDelete(object sender, ElapsedEventArgs e)
        {
            if (_firstDeleteTimer != null)
            {
                _firstDeleteTimer.Stop();
                if (_deletePressed)
                {
                    PowerToysTelemetry.Log.WriteEvent(new LauncherFirstDeleteEvent());
                }
            }
        }

        public MainWindow()
        {
            InitializeComponent();
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            _viewModel.Save();
        }

        private void BringProcessToForeground()
        {
            // Use SendInput hack to allow Activate to work - required to resolve focus issue https://github.com/microsoft/PowerToys/issues/4270
            WindowsInteropHelper.INPUT input = new WindowsInteropHelper.INPUT { Type = WindowsInteropHelper.INPUTTYPE.INPUTMOUSE, Data = { } };
            WindowsInteropHelper.INPUT[] inputs = new WindowsInteropHelper.INPUT[] { input };

            // Send empty mouse event. This makes this thread the last to send input, and hence allows it to pass foreground permission checks
            _ = NativeMethods.SendInput(1, inputs, WindowsInteropHelper.INPUT.Size);
            Activate();
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            WindowsInteropHelper.DisableControlBox(this);
            InitializePosition();

            SearchBox.QueryTextBox.DataContext = _viewModel;
            SearchBox.QueryTextBox.PreviewKeyDown += Launcher_KeyDown;
            SearchBox.QueryTextBox.TextChanged += QueryTextBox_TextChanged;

            // Set initial language flow direction
            SearchBox_UpdateFlowDirection();

            // Register language changed event
            InputLanguageManager.Current.InputLanguageChanged += SearchBox_InputLanguageChanged;

            SearchBox.QueryTextBox.Focus();
            SearchBox.QueryTextBox.ControlledElements.Add(ListBox.SuggestionsList);

            ListBox.DataContext = _viewModel;
            ListBox.SuggestionsList.SelectionChanged += SuggestionsList_SelectionChanged;
            ListBox.SuggestionsList.PreviewMouseLeftButtonUp += SuggestionsList_PreviewMouseLeftButtonUp;
            _viewModel.PropertyChanged += ViewModel_PropertyChanged;

            BringProcessToForeground();
        }

        private void SuggestionsList_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            var result = ((FrameworkElement)e.OriginalSource).DataContext;
            if (result != null)
            {
                // This may be null if the tapped item was one of the context buttons (run as admin etc).
                if (result is ResultViewModel resultVM)
                {
                    _viewModel.Results.SelectedItem = resultVM;
                    _viewModel.OpenResultWithMouseCommand.Execute(null);
                }
            }
        }

        private void ViewModel_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(MainViewModel.MainWindowVisibility))
            {
                if (Visibility == System.Windows.Visibility.Visible && _viewModel.MainWindowVisibility != Visibility.Hidden)
                {
                    // Not called on first launch
                    // Called when window is made visible by hotkey. Not called when the window is deactivated by clicking away
                    UpdatePosition();
                    BringProcessToForeground();

                    if (!_viewModel.LastQuerySelected)
                    {
                        _viewModel.LastQuerySelected = true;
                    }
                }
            }
            else if (e.PropertyName == nameof(MainViewModel.SystemQueryText))
            {
                _isTextSetProgrammatically = true;
                if (_viewModel.Results != null)
                {
                    SearchBox.QueryTextBox.Text = MainViewModel.GetSearchText(
                        _viewModel.Results.SelectedIndex,
                        _viewModel.SystemQueryText,
                        _viewModel.QueryText);
                }
            }
        }

        private void OnMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
            {
                DragMove();
            }
        }

        private void InitializePosition()
        {
            Top = WindowTop();
            Left = WindowLeft();
            _settings.WindowTop = Top;
            _settings.WindowLeft = Left;
        }

        private void OnDeactivated(object sender, EventArgs e)
        {
            if (_settings.HideWhenDeactivated)
            {
                // (this.FindResource("OutroStoryboard") as Storyboard).Begin();
                _viewModel.Hide();
            }
        }

        private void UpdatePosition()
        {
            if (_settings.RememberLastLaunchLocation)
            {
                Left = _settings.WindowLeft;
                Top = _settings.WindowTop;
            }
            else
            {
                Top = WindowTop();
                Left = WindowLeft();
            }
        }

        private void OnLocationChanged(object sender, EventArgs e)
        {
            if (_settings.RememberLastLaunchLocation)
            {
                _settings.WindowLeft = Left;
                _settings.WindowTop = Top;
            }
        }

        /// <summary>
        /// Calculates X co-ordinate of main window top left corner.
        /// </summary>
        /// <returns>X co-ordinate of main window top left corner</returns>
        private double WindowLeft()
        {
            var screen = GetScreen();
            var dip1 = WindowsInteropHelper.TransformPixelsToDIP(this, screen.WorkingArea.X, 0);
            var dip2 = WindowsInteropHelper.TransformPixelsToDIP(this, screen.WorkingArea.Width, 0);
            var left = ((dip2.X - ActualWidth) / 2) + dip1.X;
            return left;
        }

        private double WindowTop()
        {
            var screen = GetScreen();
            var dip1 = WindowsInteropHelper.TransformPixelsToDIP(this, 0, screen.WorkingArea.Y);
            var dip2 = WindowsInteropHelper.TransformPixelsToDIP(this, 0, screen.WorkingArea.Height);
            var top = ((dip2.Y - SearchBox.ActualHeight) / 4) + dip1.Y;
            return top;
        }

        private Screen GetScreen()
        {
            ManagedCommon.StartupPosition position = _settings.StartupPosition;
            switch (position)
            {
                case ManagedCommon.StartupPosition.PrimaryMonitor:
                    return Screen.PrimaryScreen;
                case ManagedCommon.StartupPosition.Focus:
                    IntPtr foregroundWindowHandle = NativeMethods.GetForegroundWindow();
                    Screen activeScreen = Screen.FromHandle(foregroundWindowHandle);
                    return activeScreen;
                case ManagedCommon.StartupPosition.Cursor:
                default:
                    return Screen.FromPoint(System.Windows.Forms.Cursor.Position);
            }
        }

        private void Launcher_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Tab && Keyboard.IsKeyDown(Key.LeftShift))
            {
                _viewModel.SelectPrevTabItemCommand.Execute(null);
                UpdateTextBoxToSelectedItem();
                e.Handled = true;
            }
            else if (e.Key == Key.Tab)
            {
                _viewModel.SelectNextTabItemCommand.Execute(null);
                UpdateTextBoxToSelectedItem();
                e.Handled = true;
            }
            else if (e.Key == Key.Down)
            {
                _viewModel.SelectNextItemCommand.Execute(null);
                UpdateTextBoxToSelectedItem();
                e.Handled = true;
            }
            else if (e.Key == Key.Up)
            {
                _viewModel.SelectPrevItemCommand.Execute(null);
                UpdateTextBoxToSelectedItem();
                e.Handled = true;
            }
            else if (e.Key == Key.Right)
            {
                if (SearchBox.QueryTextBox.CaretIndex == SearchBox.QueryTextBox.Text.Length)
                {
                    _viewModel.SelectNextContextMenuItemCommand.Execute(null);
                    e.Handled = true;
                }
            }
            else if (e.Key == Key.Left)
            {
                if (SearchBox.QueryTextBox.CaretIndex == SearchBox.QueryTextBox.Text.Length)
                {
                    if (_viewModel.Results != null && _viewModel.Results.IsContextMenuItemSelected())
                    {
                        _viewModel.SelectPreviousContextMenuItemCommand.Execute(null);
                        e.Handled = true;
                    }
                }
            }
            else if (e.Key == Key.PageDown)
            {
                _viewModel.SelectNextPageCommand.Execute(null);
                e.Handled = true;
            }
            else if (e.Key == Key.PageUp)
            {
                _viewModel.SelectPrevPageCommand.Execute(null);
                e.Handled = true;
            }
            else if (e.Key == Key.Back)
            {
                _deletePressed = true;
            }
            else
            {
                _viewModel.HandleContextMenu(e.Key, Keyboard.Modifiers);
            }
        }

        private void UpdateTextBoxToSelectedItem()
        {
            var itemText = _viewModel?.Results?.SelectedItem?.SearchBoxDisplayText() ?? null;
            if (!string.IsNullOrEmpty(itemText))
            {
                _viewModel.ChangeQueryText(itemText);
            }
        }

        private void SuggestionsList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ListView listview = (ListView)sender;
            _viewModel.Results.SelectedItem = (ResultViewModel)listview.SelectedItem;
            if (e.AddedItems.Count > 0 && e.AddedItems[0] != null)
            {
                try
                {
                    listview.ScrollIntoView(e.AddedItems[0]);
                }
                catch (ArgumentOutOfRangeException ex)
                {
                    // Due to virtualization being enabled for the listview, the layout system updates elements in a deferred manner using an algorithm that balances performance and concurrency.
                    // Hence, there can be a situation where the element index that we want to scroll into view is out of range for it's parent control.
                    // To mitigate this we use the UpdateLayout function, which forces layout update to ensure that the parent element contains the latest properties.
                    // However, it has a performance impact and is therefore not called each time.
                    Log.Exception("The parent element layout is not updated yet", ex, GetType());
                    listview.UpdateLayout();
                    listview.ScrollIntoView(e.AddedItems[0]);
                }
            }

            // To populate the AutoCompleteTextBox as soon as the selection is changed or set.
            // Setting it here instead of when the text is changed as there is a delay in executing the query and populating the result
            if (_viewModel.Results != null)
            {
                SearchBox.AutoCompleteTextBlock.Text = MainViewModel.GetAutoCompleteText(
                    _viewModel.Results.SelectedIndex,
                    _viewModel.Results.SelectedItem?.SearchBoxDisplayText(),
                    _viewModel.QueryText);
            }
        }

        private bool disposedValue;

        private void QueryTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            var textBox = (TextBox)sender;
            var text = textBox.Text;
            var autoCompleteText = SearchBox.AutoCompleteTextBlock.Text;

            if (MainViewModel.ShouldAutoCompleteTextBeEmpty(text, autoCompleteText))
            {
                SearchBox.AutoCompleteTextBlock.Text = string.Empty;
            }

            if (_isTextSetProgrammatically)
            {
                textBox.SelectionStart = textBox.Text.Length;
                _isTextSetProgrammatically = false;
            }
            else
            {
                _viewModel.QueryText = text;
                _viewModel.Query();
            }
        }

        private void ListBox_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Right)
            {
                e.Handled = true;
            }
        }

        private void OnVisibilityChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (Visibility == Visibility.Visible)
            {
                _deletePressed = false;
                if (_firstDeleteTimer != null)
                {
                    _firstDeleteTimer.Start();
                }

                // (this.FindResource("IntroStoryboard") as Storyboard).Begin();
                SearchBox.QueryTextBox.Focus();
                Keyboard.Focus(SearchBox.QueryTextBox);

                _settings.ActivateTimes++;

                if (!string.IsNullOrEmpty(SearchBox.QueryTextBox.Text))
                {
                    SearchBox.QueryTextBox.SelectAll();
                }

                // Log the time taken from pressing the hotkey till launcher is visible as separate events depending on if it's the first hotkey invoke or second
                if (!_coldStateHotkeyPressed)
                {
                    PowerToysTelemetry.Log.WriteEvent(new LauncherColdStateHotkeyEvent() { HotkeyToVisibleTimeMs = _viewModel.GetHotkeyEventTimeMs() });
                    _coldStateHotkeyPressed = true;
                }
                else
                {
                    PowerToysTelemetry.Log.WriteEvent(new LauncherWarmStateHotkeyEvent() { HotkeyToVisibleTimeMs = _viewModel.GetHotkeyEventTimeMs() });
                }
            }
            else
            {
                if (_firstDeleteTimer != null)
                {
                    _firstDeleteTimer.Stop();
                }
            }
        }

        private void OutroStoryboard_Completed(object sender, EventArgs e)
        {
            Hide();
        }

        private void SearchBox_UpdateFlowDirection()
        {
            SearchBox.QueryTextBox.FlowDirection = MainViewModel.GetLanguageFlowDirection();
            SearchBox.AutoCompleteTextBlock.FlowDirection = MainViewModel.GetLanguageFlowDirection();
        }

        private void SearchBox_InputLanguageChanged(object sender, InputLanguageEventArgs e)
        {
            SearchBox_UpdateFlowDirection();
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    if (_firstDeleteTimer != null)
                    {
                        _firstDeleteTimer.Dispose();
                    }
                }

                // TODO: free unmanaged resources (unmanaged objects) and override finalizer
                // TODO: set large fields to null
                _firstDeleteTimer = null;
                disposedValue = true;
            }
        }

        // // TODO: override finalizer only if 'Dispose(bool disposing)' has code to free unmanaged resources
        // ~MainWindow()
        // {
        //     // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
        //     Dispose(disposing: false);
        // }
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
    }
}
