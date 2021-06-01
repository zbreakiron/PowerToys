﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using Microsoft.PowerToys.Settings.UI.Helpers;
using Microsoft.PowerToys.Settings.UI.Library;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace Microsoft.PowerToys.Settings.UI.Controls
{
    public sealed partial class HotkeySettingsControl : UserControl, IDisposable
    {
        private readonly UIntPtr ignoreKeyEventFlag = (UIntPtr)0x5555;

        private bool _shiftKeyDownOnEntering;

        private bool _shiftToggled;

        public string Header { get; set; }

        public string Keys { get; set; }

        public static readonly DependencyProperty IsActiveProperty =
            DependencyProperty.Register(
                "Enabled",
                typeof(bool),
                typeof(HotkeySettingsControl),
                null);

        private bool _enabled;

        public bool Enabled
        {
            get
            {
                return _enabled;
            }

            set
            {
                SetValue(IsActiveProperty, value);
                _enabled = value;

                if (value)
                {
                    HotkeyTextBox.IsEnabled = true;

                    // TitleText.IsActive = "True";
                    // TitleGlyph.IsActive = "True";
                }
                else
                {
                    HotkeyTextBox.IsEnabled = false;

                    // TitleText.IsActive = "False";
                    // TitleGlyph.IsActive = "False";
                }
            }
        }

        public static readonly DependencyProperty HotkeySettingsProperty =
            DependencyProperty.Register(
                "HotkeySettings",
                typeof(HotkeySettings),
                typeof(HotkeySettingsControl),
                null);

        private HotkeySettings hotkeySettings;
        private HotkeySettings internalSettings;
        private HotkeySettings lastValidSettings;
        private HotkeySettingsControlHook hook;
        private bool _isActive;
        private bool disposedValue;

        public HotkeySettings HotkeySettings
        {
            get
            {
                return hotkeySettings;
            }

            set
            {
                if (hotkeySettings != value)
                {
                    hotkeySettings = value;
                    SetValue(HotkeySettingsProperty, value);
                    HotkeyTextBox.Text = HotkeySettings.ToString();
                }
            }
        }

        public HotkeySettingsControl()
        {
            InitializeComponent();
            internalSettings = new HotkeySettings();

            HotkeyTextBox.GettingFocus += HotkeyTextBox_GettingFocus;
            HotkeyTextBox.LosingFocus += HotkeyTextBox_LosingFocus;
            HotkeyTextBox.Unloaded += HotkeyTextBox_Unloaded;
            hook = new HotkeySettingsControlHook(Hotkey_KeyDown, Hotkey_KeyUp, Hotkey_IsActive, FilterAccessibleKeyboardEvents);
        }

        private void HotkeyTextBox_Unloaded(object sender, RoutedEventArgs e)
        {
            // Dispose the HotkeySettingsControlHook object to terminate the hook threads when the textbox is unloaded
            hook.Dispose();
        }

        private void KeyEventHandler(int key, bool matchValue, int matchValueCode)
        {
            switch ((Windows.System.VirtualKey)key)
            {
                case Windows.System.VirtualKey.LeftWindows:
                case Windows.System.VirtualKey.RightWindows:
                    internalSettings.Win = matchValue;
                    break;
                case Windows.System.VirtualKey.Control:
                case Windows.System.VirtualKey.LeftControl:
                case Windows.System.VirtualKey.RightControl:
                    internalSettings.Ctrl = matchValue;
                    break;
                case Windows.System.VirtualKey.Menu:
                case Windows.System.VirtualKey.LeftMenu:
                case Windows.System.VirtualKey.RightMenu:
                    internalSettings.Alt = matchValue;
                    break;
                case Windows.System.VirtualKey.Shift:
                case Windows.System.VirtualKey.LeftShift:
                case Windows.System.VirtualKey.RightShift:
                    _shiftToggled = true;
                    internalSettings.Shift = matchValue;
                    break;
                case Windows.System.VirtualKey.Escape:
                    internalSettings = new HotkeySettings();
                    HotkeySettings = new HotkeySettings();
                    return;
                default:
                    internalSettings.Code = matchValueCode;
                    break;
            }
        }

        // Function to send a single key event to the system which would be ignored by the hotkey control.
        private void SendSingleKeyboardInput(short keyCode, uint keyStatus)
        {
            NativeKeyboardHelper.INPUT inputShift = new NativeKeyboardHelper.INPUT
            {
                type = NativeKeyboardHelper.INPUTTYPE.INPUT_KEYBOARD,
                data = new NativeKeyboardHelper.InputUnion
                {
                    ki = new NativeKeyboardHelper.KEYBDINPUT
                    {
                        wVk = keyCode,
                        dwFlags = keyStatus,

                        // Any keyevent with the extraInfo set to this value will be ignored by the keyboard hook and sent to the system instead.
                        dwExtraInfo = ignoreKeyEventFlag,
                    },
                },
            };

            NativeKeyboardHelper.INPUT[] inputs = new NativeKeyboardHelper.INPUT[] { inputShift };

            _ = NativeMethods.SendInput(1, inputs, NativeKeyboardHelper.INPUT.Size);
        }

        private bool FilterAccessibleKeyboardEvents(int key, UIntPtr extraInfo)
        {
            // A keyboard event sent with this value in the extra Information field should be ignored by the hook so that it can be captured by the system instead.
            if (extraInfo == ignoreKeyEventFlag)
            {
                return false;
            }

            // If the current key press is tab, based on the other keys ignore the key press so as to shift focus out of the hotkey control.
            if ((Windows.System.VirtualKey)key == Windows.System.VirtualKey.Tab)
            {
                // Shift was not pressed while entering and Shift is not pressed while leaving the hotkey control, treat it as a normal tab key press.
                if (!internalSettings.Shift && !_shiftKeyDownOnEntering && !internalSettings.Win && !internalSettings.Alt && !internalSettings.Ctrl)
                {
                    return false;
                }

                // Shift was not pressed while entering but it was pressed while leaving the hotkey, therefore simulate a shift key press as the system does not know about shift being pressed in the hotkey.
                else if (internalSettings.Shift && !_shiftKeyDownOnEntering && !internalSettings.Win && !internalSettings.Alt && !internalSettings.Ctrl)
                {
                    // This is to reset the shift key press within the control as it was not used within the control but rather was used to leave the hotkey.
                    internalSettings.Shift = false;

                    SendSingleKeyboardInput((short)Windows.System.VirtualKey.Shift, (uint)NativeKeyboardHelper.KeyEventF.KeyDown);

                    return false;
                }

                // Shift was pressed on entering and remained pressed, therefore only ignore the tab key so that it can be passed to the system.
                // As the shift key is already assumed to be pressed by the system while it entered the hotkey control, shift would still remain pressed, hence ignoring the tab input would simulate a Shift+Tab key press.
                else if (!internalSettings.Shift && _shiftKeyDownOnEntering && !_shiftToggled && !internalSettings.Win && !internalSettings.Alt && !internalSettings.Ctrl)
                {
                    return false;
                }

                // Shift was pressed on entering but it was released and later pressed again.
                // Ignore the tab key and the system already has the shift key pressed, therefore this would simulate Shift+Tab.
                // However, since the last shift key was only used to move out of the control, reset the status of shift within the control.
                else if (internalSettings.Shift && _shiftKeyDownOnEntering && _shiftToggled && !internalSettings.Win && !internalSettings.Alt && !internalSettings.Ctrl)
                {
                    internalSettings.Shift = false;

                    return false;
                }

                // Shift was pressed on entering and was later released.
                // The system still has shift in the key pressed status, therefore pass a Shift KeyUp message to the system, to release the shift key, therefore simulating only the Tab key press.
                else if (!internalSettings.Shift && _shiftKeyDownOnEntering && _shiftToggled && !internalSettings.Win && !internalSettings.Alt && !internalSettings.Ctrl)
                {
                    SendSingleKeyboardInput((short)Windows.System.VirtualKey.Shift, (uint)NativeKeyboardHelper.KeyEventF.KeyUp);

                    return false;
                }
            }

            return true;
        }

        private async void Hotkey_KeyDown(int key)
        {
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                KeyEventHandler(key, true, key);

                // Tab and Shift+Tab are accessible keys and should not be displayed in the hotkey control.
                if (internalSettings.Code > 0 && !internalSettings.IsAccessibleShortcut())
                {
                    HotkeyTextBox.Text = internalSettings.ToString();
                    lastValidSettings = internalSettings.Clone();
                }
            });
        }

        private async void Hotkey_KeyUp(int key)
        {
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                KeyEventHandler(key, false, 0);
            });
        }

        private bool Hotkey_IsActive()
        {
            return _isActive;
        }

        private void HotkeyTextBox_GettingFocus(object sender, RoutedEventArgs e)
        {
            // Reset the status on entering the hotkey each time.
            _shiftKeyDownOnEntering = false;
            _shiftToggled = false;

            // To keep track of the shift key, whether it was pressed on entering.
            if ((NativeMethods.GetAsyncKeyState((int)Windows.System.VirtualKey.Shift) & 0x8000) != 0)
            {
                _shiftKeyDownOnEntering = true;
            }

            _isActive = true;
        }

        private void HotkeyTextBox_LosingFocus(object sender, RoutedEventArgs e)
        {
            if (lastValidSettings != null && (lastValidSettings.IsValid() || lastValidSettings.IsEmpty()))
            {
                HotkeySettings = lastValidSettings.Clone();
            }

            HotkeyTextBox.Text = hotkeySettings.ToString();
            _isActive = false;
        }

        private void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects)
                    hook.Dispose();
                }

                // TODO: free unmanaged resources (unmanaged objects) and override finalizer
                // TODO: set large fields to null
                disposedValue = true;
            }
        }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
    }
}
