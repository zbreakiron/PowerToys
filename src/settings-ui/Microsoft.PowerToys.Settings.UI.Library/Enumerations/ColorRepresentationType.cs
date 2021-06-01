﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

namespace Microsoft.PowerToys.Settings.UI.Library.Enumerations
{
    // NOTE: don't change the order (numbers) of the enumeration entires

    /// <summary>
    /// The type of the color representation
    /// </summary>
    public enum ColorRepresentationType
    {
        /// <summary>
        /// Color presentation as hexadecimal color value without the alpha-value (e.g. #0055FF)
        /// </summary>
        HEX = 0,

        /// <summary>
        /// Color presentation as RGB color value (red[0..255], green[0..255], blue[0..255])
        /// </summary>
        RGB = 1,

        /// <summary>
        /// Color presentation as CMYK color value (cyan[0%..100%], magenta[0%..100%], yellow[0%..100%], black key[0%..100%])
        /// </summary>
        CMYK = 2,

        /// <summary>
        /// Color presentation as HSL color value (hue[0°..360°], saturation[0..100%], lightness[0%..100%])
        /// </summary>
        HSL = 3,

        /// <summary>
        /// Color presentation as HSV color value (hue[0°..360°], saturation[0%..100%], value[0%..100%])
        /// </summary>
        HSV = 4,

        /// <summary>
        /// Color presentation as HSB color value (hue[0°..360°], saturation[0%..100%], brightness[0%..100%])
        /// </summary>
        HSB = 5,

        /// <summary>
        /// Color presentation as HSI color value (hue[0°..360°], saturation[0%..100%], intensity[0%..100%])
        /// </summary>
        HSI = 6,

        /// <summary>
        /// Color presentation as HWB color value (hue[0°..360°], whiteness[0%..100%], blackness[0%..100%])
        /// </summary>
        HWB = 7,

        /// <summary>
        /// Color presentation as natural color (hue, whiteness[0%..100%], blackness[0%..100%])
        /// </summary>
        NCol = 8,
    }
}
