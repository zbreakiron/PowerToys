﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Reflection;
using System.Windows;
using System.Windows.Media;
using FancyZonesEditor.Utils;

namespace FancyZonesEditor.Models
{
    public class Monitor
    {
        public LayoutOverlayWindow Window { get; private set; }

        public LayoutSettings Settings { get; set; }

        public Device Device { get; set; }

        public Monitor(Rect bounds, Rect workArea, bool primary)
        {
            Window = new LayoutOverlayWindow();
            Settings = new LayoutSettings();
            Device = new Device(bounds, workArea, primary);

            if (App.DebugMode)
            {
                long milliseconds = DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond;
                PropertyInfo[] properties = typeof(Brushes).GetProperties();
                Window.Opacity = 0.5;
                Window.Background = (Brush)properties[milliseconds % properties.Length].GetValue(null, null);
            }

            Window.KeyUp += ((App)Application.Current).App_KeyUp;
            Window.KeyDown += ((App)Application.Current).App_KeyDown;

            Window.Left = workArea.X;
            Window.Top = workArea.Y;
            Window.Width = workArea.Width;
            Window.Height = workArea.Height;
        }

        public Monitor(string id, int dpi, Rect bounds, Rect workArea, bool primary)
            : this(bounds, workArea, primary)
        {
            Device = new Device(id, dpi, bounds, workArea, primary);
        }

        public void Scale(double scaleFactor)
        {
            Device.Scale(scaleFactor);

            var workArea = Device.WorkAreaRect;
            Window.Left = workArea.X;
            Window.Top = workArea.Y;
            Window.Width = workArea.Width;
            Window.Height = workArea.Height;
        }
    }
}
