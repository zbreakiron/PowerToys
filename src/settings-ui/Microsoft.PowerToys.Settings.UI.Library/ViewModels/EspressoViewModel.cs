﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Runtime.CompilerServices;
using Microsoft.PowerToys.Settings.UI.Library.Helpers;
using Microsoft.PowerToys.Settings.UI.Library.Interfaces;

namespace Microsoft.PowerToys.Settings.UI.Library.ViewModels
{
    public class EspressoViewModel : Observable
    {
        private GeneralSettings GeneralSettingsConfig { get; set; }

        private EspressoSettings Settings { get; set; }

        private Func<string, int> SendConfigMSG { get; }

        public EspressoViewModel(ISettingsRepository<GeneralSettings> settingsRepository, ISettingsRepository<EspressoSettings> moduleSettingsRepository, Func<string, int> ipcMSGCallBackFunc)
        {
            // To obtain the general settings configurations of PowerToys Settings.
            if (settingsRepository == null)
            {
                throw new ArgumentNullException(nameof(settingsRepository));
            }

            GeneralSettingsConfig = settingsRepository.SettingsConfig;

            // To obtain the settings configurations of Fancy zones.
            if (moduleSettingsRepository == null)
            {
                throw new ArgumentNullException(nameof(moduleSettingsRepository));
            }

            Settings = moduleSettingsRepository.SettingsConfig;

            _isEnabled = GeneralSettingsConfig.Enabled.Espresso;
            _keepDisplayOn = Settings.Properties.KeepDisplayOn;
            _mode = Settings.Properties.Mode;
            _hours = Settings.Properties.Hours;
            _minutes = Settings.Properties.Minutes;

            // set the callback functions value to hangle outgoing IPC message.
            SendConfigMSG = ipcMSGCallBackFunc;
        }

        public bool IsEnabled
        {
            get => _isEnabled;
            set
            {
                if (_isEnabled != value)
                {
                    _isEnabled = value;

                    GeneralSettingsConfig.Enabled.Espresso = value;
                    OnPropertyChanged(nameof(IsEnabled));
                    OnPropertyChanged(nameof(IsTimeConfigurationEnabled));

                    var outgoing = new OutGoingGeneralSettings(GeneralSettingsConfig);
                    SendConfigMSG(outgoing.ToString());
                    NotifyPropertyChanged();
                }
            }
        }

        public bool IsTimeConfigurationEnabled
        {
            get => _mode == EspressoMode.TIMED && _isEnabled;
        }

        public EspressoMode Mode
        {
            get => _mode;
            set
            {
                if (_mode != value)
                {
                    _mode = value;
                    OnPropertyChanged(nameof(Mode));
                    OnPropertyChanged(nameof(IsTimeConfigurationEnabled));

                    Settings.Properties.Mode = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public bool KeepDisplayOn
        {
            get => _keepDisplayOn;
            set
            {
                if (_keepDisplayOn != value)
                {
                    _keepDisplayOn = value;
                    OnPropertyChanged(nameof(KeepDisplayOn));

                    Settings.Properties.KeepDisplayOn = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public uint Hours
        {
            get => _hours;
            set
            {
                if (_hours != value)
                {
                    _hours = value;
                    OnPropertyChanged(nameof(Hours));

                    Settings.Properties.Hours = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public uint Minutes
        {
            get => _minutes;
            set
            {
                if (_minutes != value)
                {
                    _minutes = value;
                    OnPropertyChanged(nameof(Minutes));

                    Settings.Properties.Minutes = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public void NotifyPropertyChanged([CallerMemberName] string propertyName = null)
        {
            OnPropertyChanged(propertyName);
            if (SendConfigMSG != null)
            {
                SndEspressoSettings outsettings = new SndEspressoSettings(Settings);
                SndModuleSettings<SndEspressoSettings> ipcMessage = new SndModuleSettings<SndEspressoSettings>(outsettings);

                var targetMessage = ipcMessage.ToJsonString();
                SendConfigMSG(targetMessage);
            }
        }

        private bool _isEnabled;
        private uint _hours;
        private uint _minutes;
        private bool _keepDisplayOn;
        private EspressoMode _mode;
    }
}
