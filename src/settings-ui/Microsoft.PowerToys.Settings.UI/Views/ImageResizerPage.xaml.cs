﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Linq;
using Microsoft.PowerToys.Settings.UI.Library;
using Microsoft.PowerToys.Settings.UI.Library.Utilities;
using Microsoft.PowerToys.Settings.UI.Library.ViewModels;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace Microsoft.PowerToys.Settings.UI.Views
{
    public sealed partial class ImageResizerPage : Page
    {
        public ImageResizerViewModel ViewModel { get; set; }

        public ImageResizerPage()
        {
            InitializeComponent();
            var settingsUtils = new SettingsUtils();
            var resourceLoader = Windows.ApplicationModel.Resources.ResourceLoader.GetForCurrentView();
            Func<string, string> loader = (string name) =>
            {
                return resourceLoader.GetString(name);
            };

            ViewModel = new ImageResizerViewModel(settingsUtils, SettingsRepository<GeneralSettings>.GetInstance(settingsUtils), ShellPage.SendDefaultIPCMessage, loader);
            DataContext = ViewModel;
        }

        public void DeleteCustomSize(object sender, RoutedEventArgs e)
        {
            Button deleteRowButton = (Button)sender;

            // Using InvariantCulture since this is internal and expected to be numerical
            bool success = int.TryParse(deleteRowButton?.CommandParameter?.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int rowNum);
            if (success)
            {
                ViewModel.DeleteImageSize(rowNum);
            }
            else
            {
                Logger.LogError("Failed to delete custom image size.");
            }
        }

        [SuppressMessage("Design", "CA1031:Do not catch general exception types", Justification = "JSON exceptions from saving new settings should be caught and logged.")]
        private void AddSizeButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ViewModel.AddRow();
            }
            catch (Exception ex)
            {
                Logger.LogError("Exception encountered when adding a new image size.", ex);
            }
        }

        [SuppressMessage("Usage", "CA1801:Review unused parameters", Justification = "Params are required for event handler signature requirements.")]
        private void ImagesSizesListView_ContainerContentChanging(ListViewBase sender, ContainerContentChangingEventArgs args)
        {
            if (ViewModel.IsListViewFocusRequested)
            {
                // Set focus to the last item in the ListView
                int size = ImagesSizesListView.Items.Count;
                ((ListViewItem)ImagesSizesListView.ContainerFromIndex(size - 1)).Focus(FocusState.Programmatic);

                // Reset the focus requested flag
                ViewModel.IsListViewFocusRequested = false;
            }
        }
    }
}
