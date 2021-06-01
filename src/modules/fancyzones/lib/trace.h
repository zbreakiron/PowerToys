#pragma once

struct Settings;
interface IZoneSet;

class Trace
{
public:
    static void RegisterProvider() noexcept;
    static void UnregisterProvider() noexcept;

    class FancyZones
    {
    public:
        static void EnableFancyZones(bool enabled) noexcept;
        static void OnKeyDown(DWORD vkCode, bool win, bool control, bool inMoveSize) noexcept;
        static void DataChanged() noexcept;
        static void EditorLaunched(int value) noexcept;
        static void Error(const DWORD errorCode, std::wstring errorMessage, std::wstring methodName) noexcept;
        static void QuickLayoutSwitched(bool shortcutUsed) noexcept;
    };

    static void SettingsChanged(const Settings& settings) noexcept;
    static void VirtualDesktopChanged() noexcept;

    class ZoneWindow
    {
    public:
        enum class InputMode
        {
            Keyboard,
            Mouse
        };

        static void KeyUp(WPARAM wparam) noexcept;
        static void MoveSizeEnd(_In_opt_ winrt::com_ptr<IZoneSet> activeSet) noexcept;
        static void CycleActiveZoneSet(_In_opt_ winrt::com_ptr<IZoneSet> activeSet, InputMode mode) noexcept;
    };
};
