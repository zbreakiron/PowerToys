// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using FancyZonesEditor.Utils;

namespace FancyZonesEditor.ViewModels
{
    public class MonitorViewModel : INotifyPropertyChanged
    {
        private const int MaxPreviewDisplaySize = 160;
        private const int MinPreviewDisplaySize = 98;

        public event PropertyChangedEventHandler PropertyChanged;

        public delegate void MonitorChangedEventHandler(MonitorChangedEventArgs args);

        public ObservableCollection<MonitorInfoModel> MonitorInfoForViewModel { get; set; }

        public static double DesktopPreviewMultiplier { get; private set; }

        public RelayCommand AddCommand { get; set; }

        public RelayCommand DeleteCommand { get; set; }

        public RelayCommand<MonitorInfoModel> SelectCommand { get; set; }

        public MonitorViewModel()
        {
            SelectCommand = new RelayCommand<MonitorInfoModel>(SelectCommandExecute, SelectCommandCanExecute);

            MonitorInfoForViewModel = new ObservableCollection<MonitorInfoModel>();
            double maxDimension = 0, minDimension = double.MaxValue;

            int i = 1;
            foreach (var monitor in App.Overlay.Monitors)
            {
                Device device = monitor.Device;
                var bounds = device.ScaledBounds;
                maxDimension = System.Math.Max(System.Math.Max(maxDimension, bounds.Height), bounds.Width);
                minDimension = System.Math.Min(System.Math.Min(minDimension, bounds.Height), bounds.Width);

                MonitorInfoForViewModel.Add(new MonitorInfoModel(i, (int)bounds.Height, (int)bounds.Width, device.Dpi, App.Overlay.CurrentDesktop == i - 1));
                i++;
            }

            double maxMultiplier = MaxPreviewDisplaySize / maxDimension;
            double minMultiplier = MinPreviewDisplaySize / minDimension;
            DesktopPreviewMultiplier = (minMultiplier + maxMultiplier) / 2.5;
        }

        private void RaisePropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private bool SelectCommandCanExecute(MonitorInfoModel monitorInfo)
        {
            return true;
        }

        private void SelectCommandExecute(MonitorInfoModel monitorInfo)
        {
            MonitorInfoForViewModel[App.Overlay.CurrentDesktop].Selected = false;
            MonitorInfoForViewModel[monitorInfo.Index - 1].Selected = true;

            App.Overlay.CurrentDesktop = monitorInfo.Index - 1;
        }
    }
}
