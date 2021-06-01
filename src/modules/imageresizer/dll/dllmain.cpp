#include "pch.h"
#include "Generated Files/resource.h"
#include "ImageResizerExt_i.h"
#include "dllmain.h"
#include <interface/powertoy_module_interface.h>
#include <common/SettingsAPI/settings_objects.h>
#include <common/utils/resources.h>
#include "Settings.h"
#include "trace.h"
#include <imageresizer/dll/ImageResizerConstants.h>

CImageResizerExtModule _AtlModule;
HINSTANCE g_hInst_imageResizer = 0;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst_imageResizer = hInstance;
        Trace::RegisterProvider();
        break;
    case DLL_PROCESS_DETACH:
        Trace::UnregisterProvider();
        break;
    }
    return _AtlModule.DllMain(dwReason, lpReserved);
}

class ImageResizerModule : public PowertoyModuleIface
{
private:
    // Enabled by default
    bool m_enabled = true;
    std::wstring app_name;
    //contains the non localized key of the powertoy
    std::wstring app_key;

public:
    // Constructor
    ImageResizerModule()
    {
        m_enabled = CSettingsInstance().GetEnabled();
        app_name = GET_RESOURCE_STRING(IDS_IMAGERESIZER);
        app_key = ImageResizerConstants::ModuleKey;
    };

    // Destroy the powertoy and free memory
    virtual void destroy() override
    {
        delete this;
    }

    // Return the localized display name of the powertoy
    virtual const wchar_t* get_name() override
    {
        return app_name.c_str();
    }

    // Return the non localized key of the powertoy, this will be cached by the runner
    virtual const wchar_t* get_key() override
    {
        return app_key.c_str();
    }

    // Return JSON with the configuration options.
    virtual bool get_config(wchar_t* buffer, int* buffer_size) override
    {
        HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

        // Create a Settings object.
        PowerToysSettings::Settings settings(hinstance, get_name());
        settings.set_description(GET_RESOURCE_STRING(IDS_SETTINGS_DESCRIPTION));
        settings.set_overview_link(L"https://aka.ms/PowerToysOverview_ImageResizer");
        settings.set_icon_key(L"pt-image-resizer");
        settings.add_header_szLarge(L"imageresizer_settingsheader", GET_RESOURCE_STRING(IDS_SETTINGS_HEADER_DESCRIPTION), GET_RESOURCE_STRING(IDS_SETTINGS_HEADER));
        return settings.serialize_to_buffer(buffer, buffer_size);
    }

    // Signal from the Settings editor to call a custom action.
    // This can be used to spawn more complex editors.
    virtual void call_custom_action(const wchar_t* action) override {}

    // Called by the runner to pass the updated settings values as a serialized JSON.
    virtual void set_config(const wchar_t* config) override {}

    // Enable the powertoy
    virtual void enable()
    {
        m_enabled = true;
        CSettingsInstance().SetEnabled(m_enabled);
        Trace::EnableImageResizer(m_enabled);
    }

    // Disable the powertoy
    virtual void disable()
    {
        m_enabled = false;
        CSettingsInstance().SetEnabled(m_enabled);
        Trace::EnableImageResizer(m_enabled);
    }

    // Returns if the powertoys is enabled
    virtual bool is_enabled() override
    {
        return m_enabled;
    }
};

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create()
{
    return new ImageResizerModule();
}