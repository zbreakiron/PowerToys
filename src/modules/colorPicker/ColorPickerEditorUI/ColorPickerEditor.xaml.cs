using Microsoft.Toolkit.Uwp;
using Microsoft.Toolkit.Uwp.Helpers;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;
using ColorHelper = Microsoft.Toolkit.Uwp.Helpers.ColorHelper;

namespace ColorPickerEditorUI
{
    public sealed partial class ColorPickerEditor : UserControl
    {
        private DispatcherTimer _timer = new DispatcherTimer();

        public event EventHandler<Color> OpenColorPickerPopupClicked;
        public event EventHandler CloseApplicationClicked;
        public event EventHandler OpenColorPickerClicked;
        public event EventHandler<string> ColorCopied;

        public ObservableCollection<Color> PickedColors { get; set; }

        public ColorPickerEditor()
        {
            InitializeComponent();
            PickedColors = new ObservableCollection<Color>();
            BackdropShadow.Receivers.Add(ShadowCastingGrid);
        }

        public void SelectFirstColor()
        {
            if(PickedColors.Count > 0)
            {
                ColorsListView.SelectedIndex = 0;
            }
        }

        private void ColorsListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var y = (sender as ListView).SelectedItem;
            if (y != null)
            {
                Color textBlock = (Color)(sender as ListView).SelectedItem;
                RenderColor(textBlock);
            }
        }

        private void RenderColor(Color SelectedColor)
        {
            ColorRect.Background = new SolidColorBrush(SelectedColor);
            HEX.ColorCode = "#" + SelectedColor.ToHex().Remove(0, 3);
            RGB.ColorCode = SelectedColor.R + "  " + SelectedColor.G + "  " + SelectedColor.B;

            HslColor SelectedHSLColor = SelectedColor.ToHsl();
            HSL.ColorCode = Math.Round(SelectedHSLColor.H, 0) + "°  " + Math.Round(SelectedHSLColor.S, 0) + "%  " + Math.Round(SelectedHSLColor.L, 0) + "%";

            HsvColor SelectedHSVColor = SelectedColor.ToHsv();
            HSB.ColorCode = Math.Round(SelectedHSVColor.H, 0) + "°  " + Math.Round(SelectedHSVColor.S, 0) + "%  " + Math.Round(SelectedHSVColor.V, 0) + "%";

            XAML.ColorCode = SelectedColor.ToHex();
            WPF.ColorCode = "Color.FromRGB(" + SelectedColor.R + ", " + SelectedColor.G + ", " + SelectedColor.B + ")";
            UWP.ColorCode = "new Color() { R = " + SelectedColor.R + ", G = " + SelectedColor.G + ", B = " + SelectedColor.B + "}";
            Maui.ColorCode = "Color.FromRGB(" + SelectedColor.R + ", " + SelectedColor.G + ", " + SelectedColor.B + ")";

            CSSHEX.ColorCode = "#" + SelectedColor.ToHex().Remove(0, 3);
            CSSRGB.ColorCode = "rgb(" + SelectedColor.R + ", " + SelectedColor.G + ", " + SelectedColor.B + ")";
            CSSHSL.ColorCode = "hsl(" + SelectedColor.R + ", " + SelectedColor.G + "%, " + SelectedColor.B + "%)";

            var hueCoeficient = 0;
            var hueCoeficient2 = 0;
            if (1 - SelectedHSVColor.V < 0.15)
            {
                hueCoeficient = 1;
            }
            if (SelectedHSVColor.V - 0.3 < 0)
            {
                hueCoeficient2 = 1;
            }

            var s = SelectedHSVColor.S;

            Gradient1.Fill = new SolidColorBrush(ColorHelper.FromHsv(Math.Min(SelectedHSVColor.H + hueCoeficient * 8, 360), s, Math.Min(SelectedHSVColor.V + 0.3, 1)));
            Gradient2.Fill = new SolidColorBrush(ColorHelper.FromHsv(Math.Min(SelectedHSVColor.H + hueCoeficient * 4, 360), s, Math.Min(SelectedHSVColor.V + 0.15, 1)));

            Gradient3.Fill = new SolidColorBrush(ColorHelper.FromHsv(Math.Min(SelectedHSVColor.H - hueCoeficient2 * 4, 360), s, Math.Max(SelectedHSVColor.V - 0.2, 0)));
            Gradient4.Fill = new SolidColorBrush(ColorHelper.FromHsv(Math.Min(SelectedHSVColor.H - hueCoeficient2 * 8, 360), s, Math.Max(SelectedHSVColor.V - 0.3, 0)));
        }

        private void CopyToClipboard(object sender, string colorCode)
        {
            CopyToClipboardBanner.Visibility = Visibility.Visible;
            
            _timer.Tick += Timer_Tick;
            _timer.Interval = new TimeSpan(0, 0, 2);
            _timer.Start();
            ColorCopied?.Invoke(this, colorCode);
        }

        private void Timer_Tick(object sender, object e)
        {
            CopyToClipboardBanner.Visibility = Visibility.Collapsed;
            _timer.Stop();
        }

        private void Gradient_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            Rectangle R = sender as Rectangle;

            AddColor(((SolidColorBrush)R.Fill).Color);
        }

        public void AddColor(Color C)
        {
            PickedColors.Insert(0, C);
            SelectFirstColor();
        }

        private void openColorPickerButton_Click(object sender, RoutedEventArgs e)
        {
            OpenColorPickerPopupClicked?.Invoke(this, ((SolidColorBrush)ColorRect.Background).Color);
        }

        private void closeButton_Click(object sender, RoutedEventArgs e)
        {
            CloseApplicationClicked?.Invoke(this, EventArgs.Empty);
        }

        private void ColorPickerButton_Click(object sender, RoutedEventArgs e)
        {
            OpenColorPickerClicked?.Invoke(this, EventArgs.Empty);
        }
    }
}
