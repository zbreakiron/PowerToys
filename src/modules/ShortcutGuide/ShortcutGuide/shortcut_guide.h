#pragma once
#include "../interface/powertoy_module_interface.h"
//#include <interface/powertoy_module_interface.h>
#include "overlay_window.h"
#include "native_event_waiter.h"
#include "ShortcutGuideSettings.h"
#include "ShortcutGuideConstants.h"

#include "Generated Files/resource.h"

// We support only one instance of the overlay
extern class OverlayWindow* instance;

class TargetState;

enum class HideWindowType
{
    ESC_PRESSED,
    WIN_RELEASED,
    WIN_SHORTCUT_PRESSED,
    THE_SHORTCUT_PRESSED
};

class OverlayWindow
{
public:
    OverlayWindow(HWND activeWindow);
    void ShowWindow();
    void CloseWindow(HideWindowType type, int mainThreadId = 0);
    bool IsDisabled();

    void on_held();
    void quick_hide();
    void was_hidden();

    bool overlay_visible() const;

    bool is_disabled_app(wchar_t* exePath);

    void get_exe_path(HWND window, wchar_t* exePath);
    ~OverlayWindow();
    static ShortcutGuideSettings GetSettings() noexcept;
private:
    std::wstring app_name;
    //contains the non localized key of the powertoy
    static inline std::wstring app_key = ShortcutGuideConstants::ModuleKey;
    std::unique_ptr<TargetState> target_state;
    std::unique_ptr<D2DOverlayWindow> winkey_popup;
    std::unique_ptr<NativeEventWaiter> event_waiter;
    std::vector<std::wstring> disabled_apps_array;
    void init_settings();
    void update_disabled_apps();
    HWND activeWindow;
    HHOOK keyboardHook;

    struct OverlayOpacity
    {
        static inline PCWSTR name = L"overlay_opacity";
        int value;
        int resourceId = IDS_SETTING_DESCRIPTION_OVERLAY_OPACITY;
    } overlayOpacity;

    struct Theme
    {
        static inline PCWSTR name = L"theme";
        std::wstring value;
        int resourceId = IDS_SETTING_DESCRIPTION_THEME;
        std::vector<std::pair<std::wstring, UINT>> keys_and_texts = {
            { L"system", IDS_SETTING_DESCRIPTION_THEME_SYSTEM },
            { L"light", IDS_SETTING_DESCRIPTION_THEME_LIGHT },
            { L"dark", IDS_SETTING_DESCRIPTION_THEME_DARK }
        };
    } theme;

    struct DisabledApps
    {
        static inline PCWSTR name = L"disabled_apps";
        std::wstring value;
    } disabledApps;

    struct OpenShortcut
    {
        static inline PCWSTR name = L"open_shortcutguide";
    } openShortcut;
};
