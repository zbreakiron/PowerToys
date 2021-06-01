#include "pch.h"

#include <filesystem>

#include <lib/FancyZones.h>
#include <lib/FancyZonesData.h>
#include <lib/Settings.h>

#include "util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    TEST_CLASS (FancyZonesUnitTests)
    {
        HINSTANCE m_hInst;
        winrt::com_ptr<IFancyZonesSettings> m_settings;
        const std::wstring_view m_moduleName = L"FancyZonesUnitTests";
        const std::wstring_view m_modulekey = L"FancyZonesUnitTests";

        TEST_METHOD_INITIALIZE(Init)
            {
                m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);
                m_settings = MakeFancyZonesSettings(m_hInst, m_moduleName.data(), m_modulekey.data());
                Assert::IsTrue(m_settings != nullptr);
            }

        TEST_METHOD_CLEANUP(CleanUp)
        {
            std::filesystem::remove_all(PTSettingsHelper::get_module_save_folder_location(m_moduleName));
        }

            TEST_METHOD (Create)
            {
                auto actual = MakeFancyZones(m_hInst, m_settings, nullptr);
                Assert::IsNotNull(actual.get());
            }
            TEST_METHOD (CreateWithEmptyHinstance)
            {
                auto actual = MakeFancyZones({}, m_settings, nullptr);
                Assert::IsNotNull(actual.get());
            }

            TEST_METHOD (CreateWithNullHinstance)
            {
                auto actual = MakeFancyZones(nullptr, m_settings, nullptr);
                Assert::IsNotNull(actual.get());
            }

            TEST_METHOD (CreateWithNullSettings)
            {
                auto actual = MakeFancyZones(m_hInst, nullptr, nullptr);
                Assert::IsNull(actual.get());
            }
    };

    TEST_CLASS (FancyZonesIZoneWindowHostUnitTests)
    {
        HINSTANCE m_hInst{};
        std::wstring m_moduleName = L"FancyZonesUnitTests";
        std::wstring m_moduleKey = L"FancyZonesUnitTests";
        winrt::com_ptr<IFancyZonesSettings> m_settings = nullptr;
        winrt::com_ptr<IZoneWindowHost> m_zoneWindowHost = nullptr;

        std::wstring serializedPowerToySettings(const Settings& settings)
        {
            PowerToysSettings::Settings ptSettings(HINSTANCE{}, L"FancyZonesUnitTests");

            ptSettings.add_hotkey(L"fancyzones_editor_hotkey", IDS_SETTING_LAUNCH_EDITOR_HOTKEY_LABEL, settings.editorHotkey);
            ptSettings.add_bool_toggle(L"fancyzones_shiftDrag", IDS_SETTING_DESCRIPTION_SHIFTDRAG, settings.shiftDrag);
            ptSettings.add_bool_toggle(L"fancyzones_mouseSwitch", IDS_SETTING_DESCRIPTION_MOUSESWITCH, settings.mouseSwitch);
            ptSettings.add_bool_toggle(L"fancyzones_overrideSnapHotkeys", IDS_SETTING_DESCRIPTION_OVERRIDE_SNAP_HOTKEYS, settings.overrideSnapHotkeys);
            ptSettings.add_bool_toggle(L"fancyzones_moveWindowAcrossMonitors", IDS_SETTING_DESCRIPTION_MOVE_WINDOW_ACROSS_MONITORS, settings.moveWindowAcrossMonitors);
            ptSettings.add_bool_toggle(L"fancyzones_moveWindowsBasedOnPosition", IDS_SETTING_DESCRIPTION_MOVE_WINDOWS_BASED_ON_POSITION, settings.moveWindowsBasedOnPosition);
            ptSettings.add_bool_toggle(L"fancyzones_zoneSetChange_flashZones", IDS_SETTING_DESCRIPTION_ZONESETCHANGE_FLASHZONES, settings.zoneSetChange_flashZones);
            ptSettings.add_bool_toggle(L"fancyzones_displayChange_moveWindows", IDS_SETTING_DESCRIPTION_DISPLAYCHANGE_MOVEWINDOWS, settings.displayChange_moveWindows);
            ptSettings.add_bool_toggle(L"fancyzones_zoneSetChange_moveWindows", IDS_SETTING_DESCRIPTION_ZONESETCHANGE_MOVEWINDOWS, settings.zoneSetChange_moveWindows);
            ptSettings.add_bool_toggle(L"fancyzones_appLastZone_moveWindows", IDS_SETTING_DESCRIPTION_APPLASTZONE_MOVEWINDOWS, settings.appLastZone_moveWindows);
            ptSettings.add_bool_toggle(L"fancyzones_restoreSize", IDS_SETTING_DESCRIPTION_RESTORESIZE, settings.restoreSize);
            ptSettings.add_bool_toggle(L"fancyzones_quickLayoutSwitch", IDS_SETTING_DESCRIPTION_QUICKLAYOUTSWITCH, settings.quickLayoutSwitch);
            ptSettings.add_bool_toggle(L"fancyzones_flashZonesOnQuickSwitch", IDS_SETTING_DESCRIPTION_FLASHZONESONQUICKSWITCH, settings.flashZonesOnQuickSwitch);
            ptSettings.add_bool_toggle(L"use_cursorpos_editor_startupscreen", IDS_SETTING_DESCRIPTION_USE_CURSORPOS_EDITOR_STARTUPSCREEN, settings.use_cursorpos_editor_startupscreen);
            ptSettings.add_bool_toggle(L"fancyzones_show_on_all_monitors", IDS_SETTING_DESCRIPTION_SHOW_FANCY_ZONES_ON_ALL_MONITORS, settings.showZonesOnAllMonitors);
            ptSettings.add_bool_toggle(L"fancyzones_multi_monitor_mode", IDS_SETTING_DESCRIPTION_SPAN_ZONES_ACROSS_MONITORS, settings.spanZonesAcrossMonitors);
            ptSettings.add_bool_toggle(L"fancyzones_makeDraggedWindowTransparent", IDS_SETTING_DESCRIPTION_MAKE_DRAGGED_WINDOW_TRANSPARENT, settings.makeDraggedWindowTransparent);
            ptSettings.add_int_spinner(L"fancyzones_highlight_opacity", IDS_SETTINGS_HIGHLIGHT_OPACITY, settings.zoneHighlightOpacity, 0, 100, 1);
            ptSettings.add_color_picker(L"fancyzones_zoneColor", IDS_SETTING_DESCRIPTION_ZONECOLOR, settings.zoneColor);
            ptSettings.add_color_picker(L"fancyzones_zoneBorderColor", IDS_SETTING_DESCRIPTION_ZONE_BORDER_COLOR, settings.zoneBorderColor);
            ptSettings.add_color_picker(L"fancyzones_zoneHighlightColor", IDS_SETTING_DESCRIPTION_ZONEHIGHLIGHTCOLOR, settings.zoneHighlightColor);
            ptSettings.add_multiline_string(L"fancyzones_excluded_apps", IDS_SETTING_EXCLUDED_APPS_DESCRIPTION, settings.excludedApps);

            return ptSettings.serialize();
        }

        TEST_METHOD_INITIALIZE(Init)
            {
                m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);
                m_settings = MakeFancyZonesSettings(m_hInst, m_moduleName.c_str(), m_moduleKey.c_str());
                Assert::IsTrue(m_settings != nullptr);

                auto fancyZones = MakeFancyZones(m_hInst, m_settings, nullptr);
                Assert::IsTrue(fancyZones != nullptr);

                m_zoneWindowHost = fancyZones.as<IZoneWindowHost>();
                Assert::IsTrue(m_zoneWindowHost != nullptr);
            }

            TEST_METHOD_CLEANUP(Cleanup)
                {
                    auto settingsFolder = PTSettingsHelper::get_module_save_folder_location(m_moduleName);
                    const auto settingsFile = settingsFolder + L"\\settings.json";
                    std::filesystem::remove(settingsFile);
                    std::filesystem::remove(settingsFolder);
                }

                TEST_METHOD (GetZoneColor)
                {
                    const auto expected = RGB(171, 175, 238);
                    const Settings settings{
                        .shiftDrag = true,
                        .mouseSwitch = true,
                        .displayChange_moveWindows = true,
                        .zoneSetChange_flashZones = false,
                        .zoneSetChange_moveWindows = true,
                        .overrideSnapHotkeys = false,
                        .moveWindowAcrossMonitors = false,
                        .moveWindowsBasedOnPosition = false,
                        .appLastZone_moveWindows = true,
                        .restoreSize = false,
                        .use_cursorpos_editor_startupscreen = true,
                        .spanZonesAcrossMonitors = false,
                        .zoneColor = L"#abafee",
                        .zoneBorderColor = L"FAFAFA",
                        .zoneHighlightColor = L"#FAFAFA",
                        .zoneHighlightOpacity = 45,
                        .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, false, false, false, VK_OEM_3),
                        .excludedApps = L"app\r\napp2",
                        .excludedAppsArray = { L"APP", L"APP2" },
                    };

                    auto config = serializedPowerToySettings(settings);
                    m_settings->SetConfig(config.c_str());

                    const auto actual = m_zoneWindowHost->GetZoneColor();
                    Assert::AreEqual(expected, actual);
                }

                TEST_METHOD (GetZoneBorderColor)
                {
                    const auto expected = RGB(171, 175, 238);
                    const Settings settings{
                        .shiftDrag = true,
                        .mouseSwitch = true,
                        .displayChange_moveWindows = true,
                        .zoneSetChange_flashZones = false,
                        .zoneSetChange_moveWindows = true,
                        .overrideSnapHotkeys = false,
                        .moveWindowAcrossMonitors = false,
                        .moveWindowsBasedOnPosition = false,
                        .appLastZone_moveWindows = true,
                        .restoreSize = false,
                        .use_cursorpos_editor_startupscreen = true,
                        .spanZonesAcrossMonitors = false,
                        .zoneColor = L"#FAFAFA",
                        .zoneBorderColor = L"#abafee",
                        .zoneHighlightColor = L"#FAFAFA",
                        .zoneHighlightOpacity = 45,
                        .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, false, false, false, VK_OEM_3),
                        .excludedApps = L"app\r\napp2",
                        .excludedAppsArray = { L"APP", L"APP2" },
                    };

                    auto config = serializedPowerToySettings(settings);
                    m_settings->SetConfig(config.c_str());

                    const auto actual = m_zoneWindowHost->GetZoneBorderColor();
                    Assert::AreEqual(expected, actual);
                }

                TEST_METHOD (GetZoneHighlightColor)
                {
                    const auto expected = RGB(171, 175, 238);
                    const Settings settings{
                        .shiftDrag = true,
                        .mouseSwitch = true,
                        .displayChange_moveWindows = true,
                        .zoneSetChange_flashZones = false,
                        .zoneSetChange_moveWindows = true,
                        .overrideSnapHotkeys = false,
                        .moveWindowAcrossMonitors = false,
                        .moveWindowsBasedOnPosition = false,
                        .appLastZone_moveWindows = true,
                        .restoreSize = false,
                        .use_cursorpos_editor_startupscreen = true,
                        .showZonesOnAllMonitors = false,
                        .spanZonesAcrossMonitors = false,
                        .makeDraggedWindowTransparent = true,
                        .zoneColor = L"#FAFAFA",
                        .zoneBorderColor = L"FAFAFA",
                        .zoneHighlightColor = L"#abafee",
                        .zoneHighlightOpacity = 45,
                        .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, false, false, false, VK_OEM_3),
                        .excludedApps = L"app\r\napp2",
                        .excludedAppsArray = { L"APP", L"APP2" },
                    };

                    auto config = serializedPowerToySettings(settings);
                    m_settings->SetConfig(config.c_str());

                    const auto actual = m_zoneWindowHost->GetZoneHighlightColor();
                    Assert::AreEqual(expected, actual);
                }

                TEST_METHOD (GetZoneHighlightOpacity)
                {
                    const auto expected = 88;
                    const Settings settings{
                        .shiftDrag = true,
                        .mouseSwitch = true,
                        .displayChange_moveWindows = true,
                        .zoneSetChange_flashZones = false,
                        .zoneSetChange_moveWindows = true,
                        .overrideSnapHotkeys = false,
                        .moveWindowAcrossMonitors = false,
                        .moveWindowsBasedOnPosition = false,
                        .appLastZone_moveWindows = true,
                        .restoreSize = false,
                        .use_cursorpos_editor_startupscreen = true,
                        .showZonesOnAllMonitors = false,
                        .spanZonesAcrossMonitors = false,
                        .makeDraggedWindowTransparent = true,
                        .zoneColor = L"#FAFAFA",
                        .zoneBorderColor = L"FAFAFA",
                        .zoneHighlightColor = L"#abafee",
                        .zoneHighlightOpacity = expected,
                        .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, false, false, false, VK_OEM_3),
                        .excludedApps = L"app\r\napp2",
                        .excludedAppsArray = { L"APP", L"APP2" },
                    };

                    auto config = serializedPowerToySettings(settings);
                    m_settings->SetConfig(config.c_str());

                    const auto actual = m_zoneWindowHost->GetZoneHighlightOpacity();
                    Assert::AreEqual(expected, actual);
                }

                TEST_METHOD (IsMakeDraggenWindowTransparentActive)
                {
                    const auto expected = true;
                    const Settings settings{
                        .shiftDrag = true,
                        .mouseSwitch = true,
                        .displayChange_moveWindows = true,
                        .zoneSetChange_flashZones = false,
                        .zoneSetChange_moveWindows = true,
                        .overrideSnapHotkeys = false,
                        .moveWindowAcrossMonitors = false,
                        .moveWindowsBasedOnPosition = false,
                        .appLastZone_moveWindows = true,
                        .restoreSize = false,
                        .use_cursorpos_editor_startupscreen = true,
                        .showZonesOnAllMonitors = false,
                        .spanZonesAcrossMonitors = false,
                        .makeDraggedWindowTransparent = true,
                        .zoneColor = L"#FAFAFA",
                        .zoneBorderColor = L"FAFAFA",
                        .zoneHighlightColor = L"#abafee",
                        .zoneHighlightOpacity = expected,
                        .editorHotkey = PowerToysSettings::HotkeyObject::from_settings(false, false, false, false, VK_OEM_3),
                        .excludedApps = L"app\r\napp2",
                        .excludedAppsArray = { L"APP", L"APP2" },
                    };

                    auto config = serializedPowerToySettings(settings);
                    m_settings->SetConfig(config.c_str());

                    Assert::AreEqual(expected, m_zoneWindowHost->isMakeDraggedWindowTransparentActive());
                }
    };

    TEST_CLASS (FancyZonesIFancyZonesCallbackUnitTests)
    {
        HINSTANCE m_hInst{};
        std::wstring m_moduleName = L"FancyZonesUnitTests";
        std::wstring m_moduleKey = L"FancyZonesUnitTests";
        winrt::com_ptr<IFancyZonesSettings> m_settings = nullptr;
        winrt::com_ptr<IFancyZonesCallback> m_fzCallback = nullptr;

        FancyZonesData& m_fancyZonesData = FancyZonesDataInstance();

        std::wstring serializedPowerToySettings(const Settings& settings)
        {
            PowerToysSettings::Settings ptSettings(HINSTANCE{}, L"FancyZonesUnitTests");

            ptSettings.add_hotkey(L"fancyzones_editor_hotkey", IDS_SETTING_LAUNCH_EDITOR_HOTKEY_LABEL, settings.editorHotkey);
            ptSettings.add_bool_toggle(L"fancyzones_shiftDrag", IDS_SETTING_DESCRIPTION_SHIFTDRAG, settings.shiftDrag);
            ptSettings.add_bool_toggle(L"fancyzones_mouseSwitch", IDS_SETTING_DESCRIPTION_MOUSESWITCH, settings.mouseSwitch);
            ptSettings.add_bool_toggle(L"fancyzones_overrideSnapHotkeys", IDS_SETTING_DESCRIPTION_OVERRIDE_SNAP_HOTKEYS, settings.overrideSnapHotkeys);
            ptSettings.add_bool_toggle(L"fancyzones_moveWindowAcrossMonitors", IDS_SETTING_DESCRIPTION_MOVE_WINDOW_ACROSS_MONITORS, settings.moveWindowAcrossMonitors);
            ptSettings.add_bool_toggle(L"fancyzones_moveWindowsBasedOnPosition", IDS_SETTING_DESCRIPTION_MOVE_WINDOWS_BASED_ON_POSITION, settings.moveWindowsBasedOnPosition);
            ptSettings.add_bool_toggle(L"fancyzones_zoneSetChange_flashZones", IDS_SETTING_DESCRIPTION_ZONESETCHANGE_FLASHZONES, settings.zoneSetChange_flashZones);
            ptSettings.add_bool_toggle(L"fancyzones_displayChange_moveWindows", IDS_SETTING_DESCRIPTION_DISPLAYCHANGE_MOVEWINDOWS, settings.displayChange_moveWindows);
            ptSettings.add_bool_toggle(L"fancyzones_zoneSetChange_moveWindows", IDS_SETTING_DESCRIPTION_ZONESETCHANGE_MOVEWINDOWS, settings.zoneSetChange_moveWindows);
            ptSettings.add_bool_toggle(L"fancyzones_appLastZone_moveWindows", IDS_SETTING_DESCRIPTION_APPLASTZONE_MOVEWINDOWS, settings.appLastZone_moveWindows);
            ptSettings.add_bool_toggle(L"fancyzones_restoreSize", IDS_SETTING_DESCRIPTION_RESTORESIZE, settings.restoreSize);
            ptSettings.add_bool_toggle(L"fancyzones_quickLayoutSwitch", IDS_SETTING_DESCRIPTION_QUICKLAYOUTSWITCH, settings.quickLayoutSwitch);
            ptSettings.add_bool_toggle(L"fancyzones_flashZonesOnQuickSwitch", IDS_SETTING_DESCRIPTION_FLASHZONESONQUICKSWITCH, settings.flashZonesOnQuickSwitch);
            ptSettings.add_bool_toggle(L"use_cursorpos_editor_startupscreen", IDS_SETTING_DESCRIPTION_USE_CURSORPOS_EDITOR_STARTUPSCREEN, settings.use_cursorpos_editor_startupscreen);
            ptSettings.add_bool_toggle(L"fancyzones_show_on_all_monitors", IDS_SETTING_DESCRIPTION_SHOW_FANCY_ZONES_ON_ALL_MONITORS, settings.showZonesOnAllMonitors);
            ptSettings.add_bool_toggle(L"fancyzones_multi_monitor_mode", IDS_SETTING_DESCRIPTION_SPAN_ZONES_ACROSS_MONITORS, settings.spanZonesAcrossMonitors);
            ptSettings.add_bool_toggle(L"fancyzones_makeDraggedWindowTransparent", IDS_SETTING_DESCRIPTION_MAKE_DRAGGED_WINDOW_TRANSPARENT, settings.makeDraggedWindowTransparent);
            ptSettings.add_int_spinner(L"fancyzones_highlight_opacity", IDS_SETTINGS_HIGHLIGHT_OPACITY, settings.zoneHighlightOpacity, 0, 100, 1);
            ptSettings.add_color_picker(L"fancyzones_zoneColor", IDS_SETTING_DESCRIPTION_ZONECOLOR, settings.zoneColor);
            ptSettings.add_color_picker(L"fancyzones_zoneBorderColor", IDS_SETTING_DESCRIPTION_ZONE_BORDER_COLOR, settings.zoneBorderColor);
            ptSettings.add_color_picker(L"fancyzones_zoneHighlightColor", IDS_SETTING_DESCRIPTION_ZONEHIGHLIGHTCOLOR, settings.zoneHighlightColor);
            ptSettings.add_multiline_string(L"fancyzones_excluded_apps", IDS_SETTING_EXCLUDED_APPS_DESCRIPTION, settings.excludedApps);

            return ptSettings.serialize();
        }

        void sendKeyboardInput(WORD code, bool release = false)
        {
            INPUT ip;
            ip.type = INPUT_KEYBOARD;
            ip.ki.wScan = 0; // hardware scan code for key
            ip.ki.time = 0;
            ip.ki.dwExtraInfo = 0;
            ip.ki.wVk = code;
            ip.ki.dwFlags = release ? KEYEVENTF_KEYUP : 0;
            SendInput(1, &ip, sizeof(INPUT));
        }

        TEST_METHOD_INITIALIZE(Init)
            {
                m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);
                m_settings = MakeFancyZonesSettings(m_hInst, m_moduleName.c_str(), m_moduleKey.c_str());
                Assert::IsTrue(m_settings != nullptr);

                auto fancyZones = MakeFancyZones(m_hInst, m_settings, nullptr);
                Assert::IsTrue(fancyZones != nullptr);

                m_fzCallback = fancyZones.as<IFancyZonesCallback>();
                Assert::IsTrue(m_fzCallback != nullptr);

                m_fancyZonesData.clear_data();
            }

            TEST_METHOD_CLEANUP(Cleanup)
                {
                    sendKeyboardInput(VK_SHIFT, true);
                    sendKeyboardInput(VK_LWIN, true);
                    sendKeyboardInput(VK_CONTROL, true);

                    auto settingsFolder = PTSettingsHelper::get_module_save_folder_location(m_moduleName);
                    const auto settingsFile = settingsFolder + L"\\settings.json";
                    std::filesystem::remove(settingsFile);
                    std::filesystem::remove(settingsFolder);
                }

                TEST_METHOD (OnKeyDownNothingPressed)
                {
                    for (DWORD code = '0'; code <= '9'; code++)
                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = code;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_LEFT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_RIGHT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }
                }

                TEST_METHOD (OnKeyDownShiftPressed)
                {
                    sendKeyboardInput(VK_SHIFT);

                    for (DWORD code = '0'; code <= '9'; code++)
                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = code;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_LEFT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_RIGHT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }
                }

                TEST_METHOD (OnKeyDownWinPressed)
                {
                    sendKeyboardInput(VK_LWIN);

                    for (DWORD code = '0'; code <= '9'; code++)
                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = code;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_LEFT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_RIGHT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }
                }

                TEST_METHOD (OnKeyDownWinShiftPressed)
                {
                    sendKeyboardInput(VK_LWIN);
                    sendKeyboardInput(VK_SHIFT);

                    for (DWORD code = '0'; code <= '9'; code++)
                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = code;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_LEFT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_RIGHT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }
                }
                /*
                TEST_METHOD (OnKeyDownWinCtrlPressed)
                {
                    sendKeyboardInput(VK_LWIN);
                    sendKeyboardInput(VK_CONTROL);

                    const Settings settings{
                        .overrideSnapHotkeys = false,
                    };

                    auto config = serializedPowerToySettings(settings);
                    m_settings->SetConfig(config.c_str());

                    for (DWORD code = '0'; code <= '9'; code++)
                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = code;
                        Assert::IsTrue(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_LEFT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }

                    {
                        tagKBDLLHOOKSTRUCT input{};
                        input.vkCode = VK_RIGHT;
                        Assert::IsFalse(m_fzCallback->OnKeyDown(&input));
                    }
                }
                */
    };
}
