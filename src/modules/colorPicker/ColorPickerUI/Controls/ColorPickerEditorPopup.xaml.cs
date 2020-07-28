using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace ColorPicker.Controls
{
    /// <summary>
    /// Interaction logic for ColorPickerEditorPopup.xaml
    /// </summary>
    public partial class ColorPickerEditorPopup : Window
    {
        public ColorPickerEditorPopup(Windows.UI.Color initialColor)
        {
            InitializeComponent();
            SelectedColor = initialColor;
        }

        public Windows.UI.Color SelectedColor;

        private void colorPicker_ChildChanged(object sender, EventArgs e)
        {
            global::Microsoft.Toolkit.Wpf.UI.XamlHost.WindowsXamlHost windowsXamlHost =
               sender as global::Microsoft.Toolkit.Wpf.UI.XamlHost.WindowsXamlHost;

            var colorPickerControl = windowsXamlHost.GetUwpInternalObject() as global::Windows.UI.Xaml.Controls.ColorPicker;

            if (colorPickerControl != null)
            {
                colorPickerControl.Color = SelectedColor;
                colorPickerControl.PointerCaptureLost += (s, e1) => {
                    SelectedColor = colorPickerControl.Color;
                    this.DialogResult = true;
                };
                this.LostFocus += ColorPickerPopupWindow_LostFocus;
            }
            else
            {
                this.LostFocus -= ColorPickerPopupWindow_LostFocus;
            }
        }

        private void ColorPickerPopupWindow_LostFocus(object sender, RoutedEventArgs e)
        {
            this.DialogResult = true;
        }
    }
}
