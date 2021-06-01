#include "pch.h"
#include "PowerRenameExt.h"
#include <interface/powertoy_module_interface.h>
#include <settings.h>
#include <trace.h>
#include <common/SettingsAPI/settings_objects.h>
#include <common/utils/resources.h>
#include "Generated Files/resource.h"
#include <atomic>
#include <dll/PowerRenameConstants.h>

std::atomic<DWORD> g_dwModuleRefCount = 0;
HINSTANCE g_hInst = 0;

class CPowerRenameClassFactory : public IClassFactory
{
public:
    CPowerRenameClassFactory(_In_ REFCLSID clsid) :
        m_refCount(1),
        m_clsid(clsid)
    {
        ModuleAddRef();
    }

    // IUnknown methods
    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void** ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CPowerRenameClassFactory, IClassFactory),
            { 0 }
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG)
    AddRef()
    {
        return ++m_refCount;
    }

    IFACEMETHODIMP_(ULONG)
    Release()
    {
        LONG refCount = --m_refCount;
        if (refCount == 0)
        {
            delete this;
        }
        return refCount;
    }

    // IClassFactory methods
    IFACEMETHODIMP CreateInstance(_In_opt_ IUnknown* punkOuter, _In_ REFIID riid, _Outptr_ void** ppv)
    {
        *ppv = NULL;
        HRESULT hr;
        if (punkOuter)
        {
            hr = CLASS_E_NOAGGREGATION;
        }
        else if (m_clsid == CLSID_PowerRenameMenu)
        {
            hr = CPowerRenameMenu::s_CreateInstance(punkOuter, riid, ppv);
        }
        else
        {
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
        return hr;
    }

    IFACEMETHODIMP LockServer(BOOL bLock)
    {
        if (bLock)
        {
            ModuleAddRef();
        }
        else
        {
            ModuleRelease();
        }
        return S_OK;
    }

private:
    ~CPowerRenameClassFactory()
    {
        ModuleRelease();
    }

    std::atomic<long> m_refCount;
    CLSID m_clsid;
};

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, void*)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        Trace::RegisterProvider();
        break;

    case DLL_PROCESS_DETACH:
        Trace::UnregisterProvider();
        break;
    }
    return TRUE;
}

//
// Checks if there are any external references to this module
//
STDAPI DllCanUnloadNow(void)
{
    return (g_dwModuleRefCount == 0) ? S_OK : S_FALSE;
}

//
// DLL export for creating COM objects
//
STDAPI DllGetClassObject(_In_ REFCLSID clsid, _In_ REFIID riid, _Outptr_ void** ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;
    CPowerRenameClassFactory* pClassFactory = new CPowerRenameClassFactory(clsid);
    hr = pClassFactory->QueryInterface(riid, ppv);
    pClassFactory->Release();
    return hr;
}

STDAPI DllRegisterServer()
{
    return S_OK;
}

STDAPI DllUnregisterServer()
{
    return S_OK;
}

void ModuleAddRef()
{
    g_dwModuleRefCount++;
}

void ModuleRelease()
{
    g_dwModuleRefCount--;
}

class PowerRenameModule : public PowertoyModuleIface
{
private:
    // Enabled by default
    bool m_enabled = true;
    std::wstring app_name;
    //contains the non localized key of the powertoy
    std::wstring app_key;

public:
    // Return the localized display name of the powertoy
    virtual PCWSTR get_name() override
    {
        return app_name.c_str();
    }

    // Return the non localized key of the powertoy, this will be cached by the runner
    virtual const wchar_t* get_key() override
    {
        return app_key.c_str();
    }

    // Enable the powertoy
    virtual void enable()
    {
        m_enabled = true;
        save_settings();
    }

    // Disable the powertoy
    virtual void disable()
    {
        m_enabled = false;
        save_settings();
    }

    // Returns if the powertoy is enabled
    virtual bool is_enabled() override
    {
        return m_enabled;
    }

    // Return JSON with the configuration options.
    // These are the settings shown on the settings page along with their current values.
    virtual bool get_config(_Out_ PWSTR buffer, _Out_ int* buffer_size) override
    {
        HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

        // Create a Settings object.
        PowerToysSettings::Settings settings(hinstance, get_name());
        settings.set_description(GET_RESOURCE_STRING(IDS_SETTINGS_DESCRIPTION));
        settings.set_icon_key(L"pt-power-rename");

        // Link to the GitHub PowerRename sub-page
        settings.set_overview_link(GET_RESOURCE_STRING(IDS_OVERVIEW_LINK));

        settings.add_bool_toggle(
            L"bool_persist_input",
            GET_RESOURCE_STRING(IDS_RESTORE_SEARCH),
            CSettingsInstance().GetPersistState());

        settings.add_bool_toggle(
            L"bool_mru_enabled",
            GET_RESOURCE_STRING(IDS_ENABLE_AUTO),
            CSettingsInstance().GetMRUEnabled());

        settings.add_int_spinner(
            L"int_max_mru_size",
            GET_RESOURCE_STRING(IDS_MAX_ITEMS),
            CSettingsInstance().GetMaxMRUSize(),
            0,
            20,
            1);

        settings.add_bool_toggle(
            L"bool_show_icon_on_menu",
            GET_RESOURCE_STRING(IDS_ICON_CONTEXT_MENU),
            CSettingsInstance().GetShowIconOnMenu());

        settings.add_bool_toggle(
            L"bool_show_extended_menu",
            GET_RESOURCE_STRING(IDS_EXTENDED_MENU_INFO),
            CSettingsInstance().GetExtendedContextMenuOnly());

        settings.add_bool_toggle(
            L"bool_use_boost_lib",
            GET_RESOURCE_STRING(IDS_USE_BOOST_LIB),
            CSettingsInstance().GetUseBoostLib());

        return settings.serialize_to_buffer(buffer, buffer_size);
    }

    // Passes JSON with the configuration settings for the powertoy.
    // This is called when the user hits Save on the settings page.
    virtual void set_config(PCWSTR config) override
    {
        try
        {
            // Parse the input JSON string.
            PowerToysSettings::PowerToyValues values =
                PowerToysSettings::PowerToyValues::from_json_string(config, get_key());

            CSettingsInstance().SetPersistState(values.get_bool_value(L"bool_persist_input").value());
            CSettingsInstance().SetMRUEnabled(values.get_bool_value(L"bool_mru_enabled").value());
            CSettingsInstance().SetMaxMRUSize(values.get_int_value(L"int_max_mru_size").value());
            CSettingsInstance().SetShowIconOnMenu(values.get_bool_value(L"bool_show_icon_on_menu").value());
            CSettingsInstance().SetExtendedContextMenuOnly(values.get_bool_value(L"bool_show_extended_menu").value());
            CSettingsInstance().SetUseBoostLib(values.get_bool_value(L"bool_use_boost_lib").value());
            CSettingsInstance().Save();

            Trace::SettingsChanged();
        }
        catch (std::exception)
        {
            // Improper JSON.
        }
    }

    // Signal from the Settings editor to call a custom action.
    // This can be used to spawn more complex editors.
    virtual void call_custom_action(const wchar_t* action) override
    {
    }

    // Destroy the powertoy and free memory
    virtual void destroy() override
    {
        delete this;
    }

    void init_settings()
    {
        m_enabled = CSettingsInstance().GetEnabled();
        Trace::EnablePowerRename(m_enabled);
    }

    void save_settings()
    {
        CSettingsInstance().SetEnabled(m_enabled);
        CSettingsInstance().Save();
        Trace::EnablePowerRename(m_enabled);
    }

    PowerRenameModule()
    {
        init_settings();
        app_name = GET_RESOURCE_STRING(IDS_POWERRENAME_APP_NAME);
        app_key = PowerRenameConstants::ModuleKey;
    }

    ~PowerRenameModule(){};
};

extern "C" __declspec(dllexport) PowertoyModuleIface* __cdecl powertoy_create()
{
    return new PowerRenameModule();
}
