#include "pch.h"
#include <WinSafer.h>
#include <Sddl.h>
#include <sstream>
#include <aclapi.h>

#include "powertoy_module.h"
#include <common/interop/two_way_pipe_message_ipc.h>
#include "tray_icon.h"
#include "general_settings.h"
#include "restart_elevated.h"
#include "update_utils.h"
#include "centralized_kb_hook.h"

#include <common/utils/json.h>
#include <common/SettingsAPI/settings_helpers.cpp>
#include <common/version/version.h>
#include <common/version/helper.h>
#include <common/logger/logger.h>
#include <common/utils/elevation.h>
#include <common/utils/process_path.h>
#include <common/utils/timeutil.h>
#include <common/utils/winapi_error.h>
#include <common/updating/updateState.h>
#include <common/themes/windows_colors.h>

#define BUFSIZE 1024

TwoWayPipeMessageIPC* current_settings_ipc = NULL;
std::atomic_bool g_isLaunchInProgress = false;

json::JsonObject get_power_toys_settings()
{
    json::JsonObject result;
    for (const auto& [name, powertoy] : modules())
    {
        try
        {
            result.SetNamedValue(name, powertoy.json_config());
        }
        catch (...)
        {
            Logger::error(L"get_power_toys_settings(): got malformed json for {} module", name);
        }
    }
    return result;
}

json::JsonObject get_all_settings()
{
    json::JsonObject result;

    result.SetNamedValue(L"general", get_general_settings().to_json());
    result.SetNamedValue(L"powertoys", get_power_toys_settings());
    return result;
}

std::optional<std::wstring> dispatch_json_action_to_module(const json::JsonObject& powertoys_configs)
{
    std::optional<std::wstring> result;
    for (const auto& powertoy_element : powertoys_configs)
    {
        const std::wstring name{ powertoy_element.Key().c_str() };
        // Currently, there is only one custom action in the general settings screen,
        // so it has to be the "restart as (non-)elevated" button.
        if (name == L"general")
        {
            try
            {
                const auto value = powertoy_element.Value().GetObjectW();
                const auto action = value.GetNamedString(L"action_name");
                if (action == L"restart_elevation")
                {
                    if (is_process_elevated())
                    {
                        schedule_restart_as_non_elevated();
                        PostQuitMessage(0);
                    }
                    else
                    {
                        schedule_restart_as_elevated(true);
                        PostQuitMessage(0);
                    }
                }
                else if (action == L"check_for_updates")
                {
                    check_for_updates_settings_callback();
                }
                else if (action == L"request_update_state_date")
                {
                    json::JsonObject json;

                    auto update_state = UpdateState::read();
                    if (update_state.githubUpdateLastCheckedDate)
                    {
                        const time_t date = *update_state.githubUpdateLastCheckedDate;
                        json.SetNamedValue(L"updateStateDate", json::value(std::to_wstring(date)));
                    }

                    result.emplace(json.Stringify());
                }
            }
            catch (...)
            {
            }
        }
        else if (modules().find(name) != modules().end())
        {
            const auto element = powertoy_element.Value().Stringify();
            modules().at(name)->call_custom_action(element.c_str());
        }
    }

    return result;
}

void send_json_config_to_module(const std::wstring& module_key, const std::wstring& settings)
{
    auto moduleIt = modules().find(module_key);
    if (moduleIt != modules().end())
    {
        moduleIt->second->set_config(settings.c_str());
        moduleIt->second.update_hotkeys();
        moduleIt->second.UpdateHotkeyEx();
    }
}

void dispatch_json_config_to_modules(const json::JsonObject& powertoys_configs)
{
    for (const auto& powertoy_element : powertoys_configs)
    {
        const auto element = powertoy_element.Value().Stringify();
        send_json_config_to_module(powertoy_element.Key().c_str(), element.c_str());
    }
};

void dispatch_received_json(const std::wstring& json_to_parse)
{
    json::JsonObject j;
    const bool ok = json::JsonObject::TryParse(json_to_parse, j);
    if (!ok)
    {
        Logger::error(L"dispatch_received_json: got malformed json: {}", json_to_parse);
        return;
    }

    for (const auto& base_element : j)
    {
        if (!current_settings_ipc)
        {
            continue;
        }

        const auto name = base_element.Key();
        const auto value = base_element.Value();

        if (name == L"general")
        {
            apply_general_settings(value.GetObjectW());
            const std::wstring settings_string{ get_all_settings().Stringify().c_str() };
            current_settings_ipc->send(settings_string);
        }
        else if (name == L"powertoys")
        {
            dispatch_json_config_to_modules(value.GetObjectW());
            const std::wstring settings_string{ get_all_settings().Stringify().c_str() };
            current_settings_ipc->send(settings_string);
        }
        else if (name == L"refresh")
        {
            const std::wstring settings_string{ get_all_settings().Stringify().c_str() };
            current_settings_ipc->send(settings_string);
        }
        else if (name == L"action")
        {
            auto result = dispatch_json_action_to_module(value.GetObjectW());
            if (result.has_value())
            {
                current_settings_ipc->send(result.value());
            }
        }
    }
    return;
}

void dispatch_received_json_callback(PVOID data)
{
    std::wstring* msg = (std::wstring*)data;
    dispatch_received_json(*msg);
    delete msg;
}

void receive_json_send_to_main_thread(const std::wstring& msg)
{
    std::wstring* copy = new std::wstring(msg);
    dispatch_run_on_main_ui_thread(dispatch_received_json_callback, copy);
}

// Try to run the Settings process with non-elevated privileges.
BOOL run_settings_non_elevated(LPCWSTR executable_path, LPWSTR executable_args, PROCESS_INFORMATION* process_info)
{
    HWND hwnd = GetShellWindow();
    if (!hwnd)
    {
        return false;
    }

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    winrt::handle process{ OpenProcess(PROCESS_CREATE_PROCESS, FALSE, pid) };
    if (!process)
    {
        return false;
    }

    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    auto pproc_buffer = std::unique_ptr<char[]>{ new (std::nothrow) char[size] };
    auto pptal = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(pproc_buffer.get());
    if (!pptal)
    {
        return false;
    }

    if (!InitializeProcThreadAttributeList(pptal, 1, 0, &size))
    {
        return false;
    }

    if (!UpdateProcThreadAttribute(pptal,
                                   0,
                                   PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
                                   &process,
                                   sizeof(process),
                                   nullptr,
                                   nullptr))
    {
        return false;
    }

    STARTUPINFOEX siex = { 0 };
    siex.lpAttributeList = pptal;
    siex.StartupInfo.cb = sizeof(siex);

    BOOL process_created = CreateProcessW(executable_path,
                                          executable_args,
                                          nullptr,
                                          nullptr,
                                          FALSE,
                                          EXTENDED_STARTUPINFO_PRESENT,
                                          nullptr,
                                          nullptr,
                                          &siex.StartupInfo,
                                          process_info);
    g_isLaunchInProgress = false;
    return process_created;
}

DWORD g_settings_process_id = 0;

void run_settings_window(bool showOobeWindow)
{
    g_isLaunchInProgress = true;

    PROCESS_INFORMATION process_info = { 0 };
    HANDLE hToken = nullptr;

    // Arguments for calling the settings executable:
    // "C:\powertoys_path\PowerToysSettings.exe" powertoys_pipe settings_pipe powertoys_pid settings_theme
    // powertoys_pipe: PowerToys pipe server.
    // settings_pipe : Settings pipe server.
    // powertoys_pid : PowerToys process pid.
    // settings_theme: pass "dark" to start the settings window in dark mode

    // Arg 1: executable path.
    std::wstring executable_path = get_module_folderpath();

    executable_path.append(L"\\Settings\\PowerToys.Settings.exe");

    // Args 2,3: pipe server. Generate unique names for the pipes, if getting a UUID is possible.
    std::wstring powertoys_pipe_name(L"\\\\.\\pipe\\powertoys_runner_");
    std::wstring settings_pipe_name(L"\\\\.\\pipe\\powertoys_settings_");
    UUID temp_uuid;
    wchar_t* uuid_chars = nullptr;
    if (UuidCreate(&temp_uuid) == RPC_S_UUID_NO_ADDRESS)
    {
        auto val = get_last_error_message(GetLastError());
        Logger::warn(L"UuidCreate can not create guid. {}", val.has_value() ? val.value() : L"");
    }
    else if (UuidToString(&temp_uuid, (RPC_WSTR*)&uuid_chars) != RPC_S_OK)
    {
        auto val = get_last_error_message(GetLastError());
        Logger::warn(L"UuidToString can not convert to string. {}", val.has_value() ? val.value() : L"");
    }

    if (uuid_chars != nullptr)
    {
        powertoys_pipe_name += std::wstring(uuid_chars);
        settings_pipe_name += std::wstring(uuid_chars);
        RpcStringFree((RPC_WSTR*)&uuid_chars);
        uuid_chars = nullptr;
    }

    // Arg 4: process pid.
    DWORD powertoys_pid = GetCurrentProcessId();

    // Arg 5: settings theme.
    const std::wstring settings_theme_setting{ get_general_settings().theme };
    std::wstring settings_theme = L"system";
    if (settings_theme_setting == L"dark" || (settings_theme_setting == L"system" && WindowsColors::is_dark_mode()))
    {
        settings_theme = L"dark";
    }

    GeneralSettings save_settings = get_general_settings();

    // Arg 6: elevated status
    bool isElevated{ get_general_settings().isElevated };
    std::wstring settings_elevatedStatus = isElevated ? L"true" : L"false";

    // Arg 7: is user an admin
    bool isAdmin{ get_general_settings().isAdmin };
    std::wstring settings_isUserAnAdmin = isAdmin ? L"true" : L"false";

    // Arg 8: should oobe window be shown
    std::wstring settings_showOobe = showOobeWindow ? L"true" : L"false";

    // create general settings file to initialize the settings file with installation configurations like :
    // 1. Run on start up.
    PTSettingsHelper::save_general_settings(save_settings.to_json());

    std::wstring executable_args = L"\"";
    executable_args.append(executable_path);
    executable_args.append(L"\" ");
    executable_args.append(powertoys_pipe_name);
    executable_args.append(L" ");
    executable_args.append(settings_pipe_name);
    executable_args.append(L" ");
    executable_args.append(std::to_wstring(powertoys_pid));
    executable_args.append(L" ");
    executable_args.append(settings_theme);
    executable_args.append(L" ");
    executable_args.append(settings_elevatedStatus);
    executable_args.append(L" ");
    executable_args.append(settings_isUserAnAdmin);
    executable_args.append(L" ");
    executable_args.append(settings_showOobe);

    BOOL process_created = false;

    if (is_process_elevated())
    {
        // TODO: Revisit this after switching to .NET 5
        // Due to a bug in .NET, running the Settings process as non-elevated
        // from an elevated process sometimes results in a crash.
        // process_created = run_settings_non_elevated(executable_path.c_str(), executable_args.data(), &process_info);
    }

    if (FALSE == process_created)
    {
        // The runner is not elevated or we failed to create the process using the
        // attribute list from Windows Explorer (this happens when PowerToys is executed
        // as Administrator from a non-Administrator user or an error occur trying).
        // In the second case the Settings process will run elevated.
        STARTUPINFO startup_info = { sizeof(startup_info) };
        if (!CreateProcessW(executable_path.c_str(),
                            executable_args.data(),
                            nullptr,
                            nullptr,
                            FALSE,
                            0,
                            nullptr,
                            nullptr,
                            &startup_info,
                            &process_info))
        {
            goto LExit;
        }
        else
        {
            g_isLaunchInProgress = false;
        }
    }

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        goto LExit;
    }

    current_settings_ipc = new TwoWayPipeMessageIPC(powertoys_pipe_name, settings_pipe_name, receive_json_send_to_main_thread);
    current_settings_ipc->start(hToken);
    g_settings_process_id = process_info.dwProcessId;

    if (process_info.hProcess)
    {
        WaitForSingleObject(process_info.hProcess, INFINITE);
        if (WaitForSingleObject(process_info.hProcess, INFINITE) != WAIT_OBJECT_0)
        {
            show_last_error_message(L"Couldn't wait on the Settings Window to close.", GetLastError(), L"PowerToys - runner");
        }
    }
    else
    {
        auto val = get_last_error_message(GetLastError());
        Logger::error(L"Process handle is empty. {}", val.has_value() ? val.value() : L"");
    }

LExit:

    if (process_info.hProcess)
    {
        CloseHandle(process_info.hProcess);
    }

    if (process_info.hThread)
    {
        CloseHandle(process_info.hThread);
    }

    if (current_settings_ipc)
    {
        current_settings_ipc->end();
        delete current_settings_ipc;
        current_settings_ipc = nullptr;
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    g_settings_process_id = 0;
}

#define MAX_TITLE_LENGTH 100
void bring_settings_to_front()
{
    auto callback = [](HWND hwnd, LPARAM data) -> BOOL {
        DWORD processId;
        if (GetWindowThreadProcessId(hwnd, &processId) && processId == g_settings_process_id)
        {
            std::wstring windowTitle = L"PowerToys Settings";

            WCHAR title[MAX_TITLE_LENGTH];
            int len = GetWindowTextW(hwnd, title, MAX_TITLE_LENGTH);
            if (len <= 0)
            {
                return TRUE;
            }
            if (wcsncmp(title, windowTitle.c_str(), len) == 0)
            {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
                return FALSE;
            }
        }

        return TRUE;
    };

    EnumWindows(callback, 0);
}

void open_settings_window()
{
    if (g_settings_process_id != 0)
    {
        bring_settings_to_front();
    }
    else
    {
        if (!g_isLaunchInProgress)
        {
            std::thread([]() {
                run_settings_window(false);
            }).detach();
        }
    }
}

void close_settings_window()
{
    if (g_settings_process_id != 0)
    {
        HANDLE proc = OpenProcess(PROCESS_TERMINATE, false, g_settings_process_id);
        if (proc != INVALID_HANDLE_VALUE)
        {
            TerminateProcess(proc, 0);
        }
    }
}

void open_oobe_window()
{
    std::thread([]() {
        run_settings_window(true);
    }).detach();
}
