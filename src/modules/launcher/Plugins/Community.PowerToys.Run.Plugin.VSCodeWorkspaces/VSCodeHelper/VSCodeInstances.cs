﻿using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace Community.PowerToys.Run.Plugin.VSCodeWorkspaces.VSCodeHelper
{
    static class VSCodeInstances
    {
        private static readonly string PathUserAppData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);

        private static string _systemPath = String.Empty;

        private static string _userAppDataPath = Environment.GetEnvironmentVariable("AppData");

        public static List<VSCodeInstance> instances = new List<VSCodeInstance>();

        private static BitmapImage Bitmap2BitmapImage(Bitmap bitmap)
        {
            using (var memory = new MemoryStream())
            {
                bitmap.Save(memory, ImageFormat.Png);
                memory.Position = 0;

                var bitmapImage = new BitmapImage();
                bitmapImage.BeginInit();
                bitmapImage.StreamSource = memory;
                bitmapImage.CacheOption = BitmapCacheOption.OnLoad;
                bitmapImage.EndInit();
                bitmapImage.Freeze();

                return bitmapImage;
            }
        }

        public static Bitmap bitmapOverlayToCenter(Bitmap bitmap1, Bitmap overlayBitmap)
        {
            int bitmap1Width = bitmap1.Width;
            int bitmap1Height = bitmap1.Height;

            Bitmap overlayBitmapResized = new Bitmap(overlayBitmap, new System.Drawing.Size(bitmap1Width / 2, bitmap1Height / 2));

            float marginLeft = (float)(bitmap1Width * 0.7 - overlayBitmapResized.Width * 0.5);
            float marginTop = (float)(bitmap1Height * 0.7 - overlayBitmapResized.Height * 0.5);

            Bitmap finalBitmap = new Bitmap(bitmap1Width, bitmap1Height);
            using (Graphics g = Graphics.FromImage(finalBitmap))
            {
                g.DrawImage(bitmap1, System.Drawing.Point.Empty);
                g.DrawImage(overlayBitmapResized, marginLeft, marginTop);
            }
            return finalBitmap;
        }

        // Gets the executablePath and AppData foreach instance of VSCode
        public static void LoadVSCodeInstances()
        {
            if (_systemPath != Environment.GetEnvironmentVariable("PATH"))
            {

                instances = new List<VSCodeInstance>();

                _systemPath = Environment.GetEnvironmentVariable("PATH");
                var paths = _systemPath.Split(";");
                paths = paths.Where(x => x.Contains("VS Code")).ToArray();
                foreach (var path in paths)
                {
                    if (Directory.Exists(path))
                    {
                        var files = Directory.GetFiles(path);
                        var iconPath = Path.GetDirectoryName(path);
                        files = files.Where(x => x.Contains("code") && !x.EndsWith(".cmd")).ToArray();

                        if (files.Length > 0)
                        {
                            var file = files[0];
                            var version = String.Empty;

                            var instance = new VSCodeInstance
                            {
                                ExecutablePath = file,
                            };

                            if (file.EndsWith("code"))
                            {
                                version = "Code";
                                instance.VSCodeVersion = VSCodeVersion.Stable;
                            }
                            else if (file.EndsWith("code-insiders"))
                            {
                                version = "Code - Insiders";
                                instance.VSCodeVersion = VSCodeVersion.Insiders;
                            }
                            else if (file.EndsWith("code-exploration"))
                            {
                                version = "Code - Exploration";
                                instance.VSCodeVersion = VSCodeVersion.Exploration;
                            }

                            if (version != String.Empty)
                            {
                                instance.AppData = Path.Combine(_userAppDataPath, version);
                                var iconVSCode = Path.Join(iconPath, $"{version}.exe");

                                var bitmapIconVscode = Icon.ExtractAssociatedIcon(iconVSCode).ToBitmap();

                                //workspace
                                var folderIcon = (Bitmap)System.Drawing.Image.FromFile(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) + "//Images//folder.png");
                                instance.WorkspaceIconBitMap = Bitmap2BitmapImage(bitmapOverlayToCenter(folderIcon, bitmapIconVscode));

                                //remote
                                var monitorIcon = (Bitmap)System.Drawing.Image.FromFile(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) + "//Images//monitor.png");

                                instance.RemoteIconBitMap = Bitmap2BitmapImage(bitmapOverlayToCenter(monitorIcon, bitmapIconVscode));

                                instances.Add(instance);
                            }
                        }
                    }
                }
            }
        }
    }
}
