#include "pch.h"
#include "Generated Files/resource.h"
#include "PowerRenameUI.h"

#include <common/utils/resources.h>
#include <common/display/dpi_aware.h>
#include <commctrl.h>
#include <Shlobj.h>
#include <helpers.h>
#include <PowerRenameEnum.h>
#include <windowsx.h>
#include <thread>
#include <trace.h>

extern HINSTANCE g_hInst;

enum
{
    MATCHMODE_FULLNAME = 0,
    MATCHMODE_NAMEONLY,
    MATCHMODE_EXTENIONONLY
};

struct FlagCheckboxMap
{
    DWORD flag;
    DWORD id;
};

FlagCheckboxMap g_flagCheckboxMap[] = {
    { UseRegularExpressions, IDC_CHECK_USEREGEX },
    { ExcludeSubfolders, IDC_CHECK_EXCLUDESUBFOLDERS },
    { EnumerateItems, IDC_CHECK_ENUMITEMS },
    { ExcludeFiles, IDC_CHECK_EXCLUDEFILES },
    { CaseSensitive, IDC_CHECK_CASESENSITIVE },
    { MatchAllOccurences, IDC_CHECK_MATCHALLOCCURENCES },
    { ExcludeFolders, IDC_CHECK_EXCLUDEFOLDERS },
    { NameOnly, IDC_CHECK_NAMEONLY },
    { ExtensionOnly, IDC_CHECK_EXTENSIONONLY },
    { Uppercase, IDC_TRANSFORM_UPPERCASE },
    { Lowercase, IDC_TRANSFORM_LOWERCASE },
    { Titlecase, IDC_TRANSFORM_TITLECASE },
    { Capitalized, IDC_TRANSFORM_CAPITALIZED }
};

struct RepositionMap
{
    DWORD id;
    DWORD flags;
};

enum
{
    Reposition_None = 0,
    Reposition_X = 0x1,
    Reposition_Y = 0x2,
    Reposition_Width = 0x4,
    Reposition_Height = 0x8
};

RepositionMap g_repositionMap[] = {
    { IDC_SEARCHREPLACEGROUP, Reposition_Width },
    { IDC_OPTIONSGROUP, Reposition_Width },
    { IDC_PREVIEWGROUP, Reposition_Width | Reposition_Height },
    { IDC_EDIT_SEARCHFOR, Reposition_Width },
    { IDC_EDIT_REPLACEWITH, Reposition_Width },
    { IDC_LIST_PREVIEW, Reposition_Width | Reposition_Height },
    { IDC_STATUS_MESSAGE_SELECTED, Reposition_Y },
    { IDC_STATUS_MESSAGE_RENAMING, Reposition_Y },
    { ID_RENAME, Reposition_X | Reposition_Y },
    { ID_ABOUT, Reposition_X | Reposition_Y },
    { IDCANCEL, Reposition_X | Reposition_Y }
};

inline int RECT_WIDTH(RECT& r)
{
    return r.right - r.left;
}
inline int RECT_HEIGHT(RECT& r)
{
    return r.bottom - r.top;
}

CPowerRenameUI::CPowerRenameUI() :
    m_refCount(1)
{
    CSettingsInstance().Load();
    (void)OleInitialize(nullptr);
    ModuleAddRef();
}

// IUnknown
IFACEMETHODIMP CPowerRenameUI::QueryInterface(__in REFIID riid, __deref_out void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CPowerRenameUI, IPowerRenameUI),
        QITABENT(CPowerRenameUI, IPowerRenameManagerEvents),
        QITABENT(CPowerRenameUI, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG)
CPowerRenameUI::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG)
CPowerRenameUI::Release()
{
    long refCount = InterlockedDecrement(&m_refCount);
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
}

HRESULT CPowerRenameUI::s_CreateInstance(_In_ IPowerRenameManager* psrm, _In_opt_ IUnknown* dataSource, _In_ bool enableDragDrop, _Outptr_ IPowerRenameUI** ppsrui)
{
    *ppsrui = nullptr;
    CPowerRenameUI* prui = new CPowerRenameUI();
    HRESULT hr = E_OUTOFMEMORY;
    if (prui)
    {
        // Pass the IPowerRenameManager to the IPowerRenameUI so it can subscribe to events
        hr = prui->_Initialize(psrm, dataSource, enableDragDrop);
        if (SUCCEEDED(hr))
        {
            hr = prui->QueryInterface(IID_PPV_ARGS(ppsrui));
        }
        prui->Release();
    }
    return hr;
}

// IPowerRenameUI
IFACEMETHODIMP CPowerRenameUI::Show(_In_opt_ HWND hwndParent)
{
    return _DoModeless(hwndParent);
}

IFACEMETHODIMP CPowerRenameUI::Close()
{
    _OnCloseDlg();
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::Update()
{
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::GetHwnd(_Out_ HWND* hwnd)
{
    *hwnd = m_hwnd;
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::GetShowUI(_Out_ bool* showUI)
{
    // Let callers know that it is OK to show UI (ex: progress dialog, error dialog and conflict dialog UI)
    *showUI = true;
    return S_OK;
}

// IPowerRenameManagerEvents
IFACEMETHODIMP CPowerRenameUI::OnItemAdded(_In_ IPowerRenameItem*)
{
    // Check if the user canceled the enumeration from the progress dialog UI
    if (m_prpui.IsCanceled())
    {
        m_prpui.Stop();
        if (m_sppre)
        {
            // Cancel the enumeration
            m_sppre->Cancel();
        }
    }

    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::OnUpdate(_In_ IPowerRenameItem*)
{
    UINT visibleItemCount = 0;
    if (m_spsrm)
    {
        m_spsrm->GetVisibleItemCount(&visibleItemCount);
    }
    m_listview.SetItemCount(visibleItemCount);
    m_listview.RedrawItems(0, visibleItemCount);
    _UpdateCounts();
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::OnError(_In_ IPowerRenameItem*)
{
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::OnRegExStarted(_In_ DWORD threadId)
{
    m_disableCountUpdate = true;
    m_currentRegExId = threadId;
    _UpdateCounts();
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::OnRegExCanceled(_In_ DWORD threadId)
{
    if (m_currentRegExId == threadId)
    {
        m_disableCountUpdate = false;
        _UpdateCounts();
    }

    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::OnRegExCompleted(_In_ DWORD threadId)
{
    // Enable list view
    if (m_currentRegExId == threadId)
    {
        m_disableCountUpdate = false;
        _UpdateCounts();
    }
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::OnRenameStarted()
{
    // Disable controls
    EnableWindow(m_hwnd, FALSE);
    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::OnRenameCompleted()
{
    // Enable controls
    EnableWindow(m_hwnd, TRUE);

    // Close the window
    PostMessage(m_hwnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
    return S_OK;
}

// IDropTarget
IFACEMETHODIMP CPowerRenameUI::DragEnter(_In_ IDataObject* pdtobj, DWORD /* grfKeyState */, POINTL pt, _Inout_ DWORD* pdwEffect)
{
    if (m_spdth)
    {
        POINT ptT = { pt.x, pt.y };
        m_spdth->DragEnter(m_hwnd, pdtobj, &ptT, *pdwEffect);
    }

    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::DragOver(DWORD /* grfKeyState */, POINTL pt, _Inout_ DWORD* pdwEffect)
{
    if (m_spdth)
    {
        POINT ptT = { pt.x, pt.y };
        m_spdth->DragOver(&ptT, *pdwEffect);
    }

    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::DragLeave()
{
    if (m_spdth)
    {
        m_spdth->DragLeave();
    }

    return S_OK;
}

IFACEMETHODIMP CPowerRenameUI::Drop(_In_ IDataObject* pdtobj, DWORD, POINTL pt, _Inout_ DWORD* pdwEffect)
{
    if (m_spdth)
    {
        POINT ptT = { pt.x, pt.y };
        m_spdth->Drop(pdtobj, &ptT, *pdwEffect);
    }

    EnableWindow(GetDlgItem(m_hwnd, ID_RENAME), TRUE);
    EnableWindow(m_hwndLV, TRUE);

    // Populate the manager from the data object
    if (m_spsrm)
    {
        _EnumerateItems(pdtobj);
    }

    return S_OK;
}

HRESULT CPowerRenameUI::_Initialize(_In_ IPowerRenameManager* psrm, _In_opt_ IUnknown* dataSource, _In_ bool enableDragDrop)
{
    // Cache the rename manager
    m_spsrm = psrm;

    // Cache the data source for enumeration later
    m_dataSource = dataSource;

    m_enableDragDrop = enableDragDrop;

    HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&m_spdth));
    if (SUCCEEDED(hr))
    {
        // Subscribe to rename manager events
        hr = m_spsrm->Advise(this, &m_cookie);
    }

    if (FAILED(hr))
    {
        _Cleanup();
    }

    return hr;
}

HRESULT CPowerRenameUI::_InitAutoComplete()
{
    HRESULT hr = S_OK;
    if (CSettingsInstance().GetMRUEnabled())
    {
        hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&m_spSearchAC));
        if (SUCCEEDED(hr))
        {
            hr = CRenameMRUSearch_CreateInstance(&m_spSearchACL);
            if (SUCCEEDED(hr))
            {
                hr = m_spSearchAC->Init(GetDlgItem(m_hwnd, IDC_EDIT_SEARCHFOR), m_spSearchACL, nullptr, nullptr);
                if (SUCCEEDED(hr))
                {
                    hr = m_spSearchAC->SetOptions(ACO_AUTOSUGGEST | ACO_AUTOAPPEND | ACO_UPDOWNKEYDROPSLIST);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&m_spReplaceAC));
            if (SUCCEEDED(hr))
            {
                hr = CRenameMRUReplace_CreateInstance(&m_spReplaceACL);
                if (SUCCEEDED(hr))
                {
                    hr = m_spReplaceAC->Init(GetDlgItem(m_hwnd, IDC_EDIT_REPLACEWITH), m_spReplaceACL, nullptr, nullptr);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_spReplaceAC->SetOptions(ACO_AUTOSUGGEST | ACO_AUTOAPPEND | ACO_UPDOWNKEYDROPSLIST);
                    }
                }
            }
        }
    }

    return hr;
}

void CPowerRenameUI::_Cleanup()
{
    if (m_spsrm && m_cookie != 0)
    {
        m_spsrm->UnAdvise(m_cookie);
        m_cookie = 0;
        m_spsrm = nullptr;
    }

    m_dataSource = nullptr;
    m_spdth = nullptr;

    if (m_enableDragDrop)
    {
        RevokeDragDrop(m_hwnd);
    }

    m_hwnd = NULL;
}

HRESULT CPowerRenameUI::_EnumerateItems(_In_ IUnknown* pdtobj)
{
    HRESULT hr = S_OK;
    // Enumerate the data object and populate the manager
    if (m_spsrm)
    {
        m_disableCountUpdate = true;

        // Ensure we re-create the enumerator
        m_sppre = nullptr;
        hr = CPowerRenameEnum::s_CreateInstance(pdtobj, m_spsrm, IID_PPV_ARGS(&m_sppre));
        if (SUCCEEDED(hr))
        {
            m_prpui.Start();
            hr = m_sppre->Start();
            m_prpui.Stop();
        }

        m_disableCountUpdate = false;

        if (SUCCEEDED(hr))
        {
            UINT itemCount = 0;
            m_spsrm->GetItemCount(&itemCount);
            m_listview.SetItemCount(itemCount);

            _UpdateCounts();
        }
    }

    return hr;
}

HRESULT CPowerRenameUI::_ReadSettings()
{
    // Check if we should read flags from settings
    // or the defaults from the manager.
    DWORD flags = 0;
    if (CSettingsInstance().GetPersistState())
    {
        flags = CSettingsInstance().GetFlags();
        m_spsrm->PutFlags(flags);

        SetDlgItemText(m_hwnd, IDC_EDIT_SEARCHFOR, CSettingsInstance().GetSearchText().c_str());
        SetDlgItemText(m_hwnd, IDC_EDIT_REPLACEWITH, CSettingsInstance().GetReplaceText().c_str());
    }
    else
    {
        m_spsrm->GetFlags(&flags);
    }

    _SetCheckboxesFromFlags(flags);

    return S_OK;
}

HRESULT CPowerRenameUI::_WriteSettings()
{
    // Check if we should store our settings
    if (CSettingsInstance().GetPersistState())
    {
        DWORD flags = 0;
        m_spsrm->GetFlags(&flags);
        CSettingsInstance().SetFlags(flags);

        wchar_t buffer[CSettings::MAX_INPUT_STRING_LEN];
        buffer[0] = L'\0';
        GetDlgItemText(m_hwnd, IDC_EDIT_SEARCHFOR, buffer, ARRAYSIZE(buffer));
        CSettingsInstance().SetSearchText(buffer);

        if (CSettingsInstance().GetMRUEnabled() && m_spSearchACL)
        {
            CComPtr<IPowerRenameMRU> spSearchMRU;
            if (SUCCEEDED(m_spSearchACL->QueryInterface(IID_PPV_ARGS(&spSearchMRU))))
            {
                spSearchMRU->AddMRUString(buffer);
            }
        }

        buffer[0] = L'\0';
        GetDlgItemText(m_hwnd, IDC_EDIT_REPLACEWITH, buffer, ARRAYSIZE(buffer));
        CSettingsInstance().SetReplaceText(buffer);

        if (CSettingsInstance().GetMRUEnabled() && m_spReplaceACL)
        {
            CComPtr<IPowerRenameMRU> spReplaceMRU;
            if (SUCCEEDED(m_spReplaceACL->QueryInterface(IID_PPV_ARGS(&spReplaceMRU))))
            {
                spReplaceMRU->AddMRUString(buffer);
            }
        }

        Trace::SettingsChanged();
    }

    return S_OK;
}

void CPowerRenameUI::_OnCloseDlg()
{
    if (m_hwnd != NULL)
    {
        if (m_modeless)
        {
            DestroyWindow(m_hwnd);
        }
        else
        {
            EndDialog(m_hwnd, 1);
        }
    }
}

void CPowerRenameUI::_OnDestroyDlg()
{
    _Cleanup();

    if (m_modeless)
    {
        PostQuitMessage(0);
    }
}

void CPowerRenameUI::_OnRename()
{
    if (m_spsrm)
    {
        m_spsrm->Rename(m_hwnd);
    }

    // Persist the current settings.  We only do this when
    // a rename is actually performed.  Not when the user
    // closes/cancels the dialog.
    _WriteSettings();
}

void CPowerRenameUI::_OnAbout()
{
    // Launch github page
    SHELLEXECUTEINFO info = { 0 };
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.lpVerb = L"open";
    info.lpFile = L"https://aka.ms/PowerToysOverview_PowerRename";
    info.nShow = SW_SHOWDEFAULT;

    ShellExecuteEx(&info);
}

HRESULT CPowerRenameUI::_DoModal(__in_opt HWND hwnd)
{
    m_modeless = false;
    HRESULT hr = S_OK;
    INT_PTR ret = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_MAIN), hwnd, s_DlgProc, (LPARAM)this);
    if (ret < 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}
void CPowerRenameUI::BecomeForegroundWindow()
{
    static INPUT i = { INPUT_MOUSE, {} };
    SendInput(1, &i, sizeof(i));
    SetForegroundWindow(m_hwnd);
}

HRESULT CPowerRenameUI::_DoModeless(__in_opt HWND hwnd)
{
    m_modeless = true;
    HRESULT hr = S_OK;
    if (NULL != CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MAIN), hwnd, s_DlgProc, (LPARAM)this))
    {
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
        BecomeForegroundWindow();
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!IsDialogMessage(m_hwnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

INT_PTR CPowerRenameUI::_DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR bRet = TRUE; // default for all handled cases in switch below

    switch (uMsg)
    {
    case WM_INITDIALOG:
        _OnInitDlg();
        break;

    case WM_COMMAND:
        _OnCommand(wParam, lParam);
        break;

    case WM_NOTIFY:
        bRet = _OnNotify(wParam, lParam);
        break;

    case WM_THEMECHANGED:
        _OnSize(SIZE_RESTORED);
        break;

    case WM_SIZE:
        _OnSize(wParam);
        break;

    case WM_GETMINMAXINFO:
        _OnGetMinMaxInfo(lParam);
        break;

    case WM_CLOSE:
        _OnCloseDlg();
        break;

    case WM_DESTROY:
        _OnDestroyDlg();
        break;

    default:
        bRet = FALSE;
    }
    return bRet;
}

void CPowerRenameUI::_OnInitDlg()
{
    // Load text in the dialog controls
    _InitDlgText();

    m_hwndLV = GetDlgItem(m_hwnd, IDC_LIST_PREVIEW);

    m_listview.Init(m_hwndLV);

    if (m_dataSource)
    {
        // Populate the manager from the data object
        if (FAILED(_EnumerateItems(m_dataSource)))
        {
            // Failed during enumeration.  Close the dialog.
            _OnCloseDlg();
            return;
        }
    }

    // Initialize from stored settings. Do this now in case we have
    // restored a previous search or replace text that needs to be
    // evaluated against the items we just enumerated.
    _ReadSettings();

    // Load the main icon
    LoadIconWithScaleDown(g_hInst, MAKEINTRESOURCE(IDI_RENAME), 32, 32, &m_iconMain);

    // Update the icon associated with our main app window
    SendMessage(m_hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)m_iconMain);
    SendMessage(m_hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)m_iconMain);

    // TODO: put this behind a setting?
    if (m_enableDragDrop)
    {
        RegisterDragDrop(m_hwnd, this);
    }

    RECT rc = { 0 };
    GetWindowRect(m_hwnd, &rc);
    m_initialWidth = RECT_WIDTH(rc);
    m_initialHeight = RECT_HEIGHT(rc);

    DPIAware::GetScreenDPIForWindow(m_hwnd, m_initialDPI);

    for (UINT u = 0; u < ARRAYSIZE(g_repositionMap); u++)
    {
        _CollectItemPosition(g_repositionMap[u].id);
    }

    _InitAutoComplete();

    // Disable rename button by default.  It will be enabled in _UpdateCounts if
    // there are times to be renamed
    EnableWindow(GetDlgItem(m_hwnd, ID_RENAME), FALSE);

    // Update UI elements that depend on number of items selected or to be renamed
    _UpdateCounts();

    m_initialized = true;
}

void UpdateDlgControl(HWND dlg, int item_id, int string_id)
{
    HWND control = GetDlgItem(dlg, item_id);
    SetWindowText(control, GET_RESOURCE_STRING(string_id).c_str());
}

void CPowerRenameUI::_InitDlgText()
{
    // load strings
    SetWindowText(m_hwnd, GET_RESOURCE_STRING(IDS_APP_TITLE).c_str());
    UpdateDlgControl(m_hwnd, IDC_CHECK_USEREGEX, IDS_USE_REGEX);
    UpdateDlgControl(m_hwnd, IDC_CHECK_CASESENSITIVE, IDS_CASE_SENSITIVE);
    UpdateDlgControl(m_hwnd, IDC_CHECK_MATCHALLOCCURENCES, IDS_MATCH_ALL);
    UpdateDlgControl(m_hwnd, IDC_TRANSFORM_UPPERCASE, IDS_MAKE_UPPERCASE);
    UpdateDlgControl(m_hwnd, IDC_CHECK_EXCLUDEFILES, IDS_EXCLUDE_FILES);
    UpdateDlgControl(m_hwnd, IDC_CHECK_EXCLUDEFOLDERS, IDS_EXCLUDE_FOLDERS);
    UpdateDlgControl(m_hwnd, IDC_CHECK_EXCLUDESUBFOLDERS, IDS_EXCLUDE_SUBFOLDER);
    UpdateDlgControl(m_hwnd, IDC_TRANSFORM_LOWERCASE, IDS_MAKE_LOWERCASE);
    UpdateDlgControl(m_hwnd, IDC_CHECK_ENUMITEMS, IDS_ENUMERATE_ITEMS);
    UpdateDlgControl(m_hwnd, IDC_CHECK_NAMEONLY, IDS_ITEM_NAME_ONLY);
    UpdateDlgControl(m_hwnd, IDC_CHECK_EXTENSIONONLY, IDS_ITEM_EXTENSION_ONLY);
    UpdateDlgControl(m_hwnd, IDC_TRANSFORM_TITLECASE, IDS_MAKE_TITLECASE);
    UpdateDlgControl(m_hwnd, IDC_TRANSFORM_CAPITALIZED, IDS_MAKE_CAPITALIZED);
    UpdateDlgControl(m_hwnd, ID_RENAME, IDS_RENAME_BUTTON);
    UpdateDlgControl(m_hwnd, ID_ABOUT, IDS_HELP_BUTTON);
    UpdateDlgControl(m_hwnd, IDCANCEL, IDS_CANCEL_BUTTON);
    UpdateDlgControl(m_hwnd, IDC_SEARCH_FOR, IDS_SEARCH_FOR);
    UpdateDlgControl(m_hwnd, IDC_REPLACE_WITH, IDS_REPLACE_WITH);
    UpdateDlgControl(m_hwnd, IDC_STATUS_MESSAGE_SELECTED, IDS_ITEMS_SELECTED);
    UpdateDlgControl(m_hwnd, IDC_STATUS_MESSAGE_RENAMING, IDS_ITEMS_RENAMING);
    UpdateDlgControl(m_hwnd, IDC_OPTIONSGROUP, IDS_OPTIONS);
    UpdateDlgControl(m_hwnd, IDC_PREVIEWGROUP, IDS_PREVIEW);
    UpdateDlgControl(m_hwnd, IDC_SEARCHREPLACEGROUP, IDS_RENAME_CRITERIA);
}

void CPowerRenameUI::_OnCommand(_In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDOK:
    case ID_RENAME:
        _OnRename();
        break;

    case ID_ABOUT:
        _OnAbout();
        break;

    case IDCANCEL:
        _OnCloseDlg();
        break;

    case IDC_EDIT_REPLACEWITH:
    case IDC_EDIT_SEARCHFOR:
        if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
        {
            _OnSearchReplaceChanged();
        }
        break;

    case IDC_CHECK_CASESENSITIVE:
    case IDC_CHECK_ENUMITEMS:
    case IDC_CHECK_EXCLUDEFILES:
    case IDC_CHECK_EXCLUDEFOLDERS:
    case IDC_CHECK_EXCLUDESUBFOLDERS:
    case IDC_CHECK_MATCHALLOCCURENCES:
    case IDC_CHECK_USEREGEX:
    case IDC_CHECK_EXTENSIONONLY:
    case IDC_CHECK_NAMEONLY:
    case IDC_TRANSFORM_UPPERCASE:
    case IDC_TRANSFORM_LOWERCASE:
    case IDC_TRANSFORM_CAPITALIZED:
    case IDC_TRANSFORM_TITLECASE:
        if (BN_CLICKED == HIWORD(wParam))
        {
            _ValidateFlagCheckbox(LOWORD(wParam));
            _GetFlagsFromCheckboxes();
        }
        break;
    }
}

BOOL CPowerRenameUI::_OnNotify(_In_ WPARAM wParam, _In_ LPARAM lParam)
{
    bool ret = FALSE;
    LPNMHDR pnmdr = (LPNMHDR)lParam;
    LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)pnmdr;
    NMLVEMPTYMARKUP* pnmMarkup = NULL;

    if (pnmdr)
    {
        BOOL checked = FALSE;
        switch (pnmdr->code)
        {
        case LVN_COLUMNCLICK:
            if (m_spsrm)
            {
                m_listview.OnColumnClick(m_spsrm, ((LPNMLISTVIEW)lParam)->iSubItem);
            }
            break;
        case HDN_ITEMSTATEICONCLICK:
            if (m_spsrm)
            {
                m_listview.ToggleAll(m_spsrm, (!(((LPNMHEADER)lParam)->pitem->fmt & HDF_CHECKED)));
                _UpdateCounts();
            }
            break;

        case LVN_GETEMPTYMARKUP:
            pnmMarkup = (NMLVEMPTYMARKUP*)lParam;
            pnmMarkup->dwFlags = EMF_CENTERED;
            LoadString(g_hInst, IDS_LISTVIEW_EMPTY, pnmMarkup->szMarkup, ARRAYSIZE(pnmMarkup->szMarkup));
            ret = TRUE;
            break;

        case LVN_BEGINLABELEDIT:
            ret = TRUE;
            break;

        case LVN_KEYDOWN:
            if (m_spsrm)
            {
                m_listview.OnKeyDown(m_spsrm, (LV_KEYDOWN*)pnmdr);
                _UpdateCounts();
            }
            break;

        case LVN_GETDISPINFO:
            if (m_spsrm)
            {
                m_listview.GetDisplayInfo(m_spsrm, (LV_DISPINFO*)pnmlv);
            }
            break;

        case NM_CLICK:
        {
            if (m_spsrm)
            {
                m_listview.OnClickList(m_spsrm, (NM_LISTVIEW*)pnmdr);
                _UpdateCounts();
            }
            break;
        }
        }
    }

    return ret;
}

void CPowerRenameUI::_OnGetMinMaxInfo(_In_ LPARAM lParam)
{
    if (m_initialWidth)
    {
        // Prevent resizing the dialog less than the original size
        MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        pMinMaxInfo->ptMinTrackSize.x = m_initialWidth;
        pMinMaxInfo->ptMinTrackSize.y = m_initialHeight;
    }
}

void CPowerRenameUI::_OnSize(_In_ WPARAM wParam)
{
    if ((wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) && m_initialWidth)
    {
        for (UINT u = 0; u < ARRAYSIZE(g_repositionMap); u++)
        {
            _MoveControl(g_repositionMap[u].id, g_repositionMap[u].flags);
        }

        m_listview.OnSize();
    }
}

void CPowerRenameUI::_MoveControl(_In_ DWORD id, _In_ DWORD repositionFlags)
{
    HWND hwnd = GetDlgItem(m_hwnd, id);

    UINT flags = SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE;
    if (!((repositionFlags & Reposition_X) || (repositionFlags & Reposition_Y)))
    {
        flags |= SWP_NOMOVE;
    }

    if (!((repositionFlags & Reposition_Width) || (repositionFlags & Reposition_Height)))
    {
        flags |= SWP_NOSIZE;
    }
    RECT rc = { 0 };
    GetWindowRect(m_hwnd, &rc);
    int mainWindowWidth = rc.right - rc.left;
    int mainWindowHeight = rc.bottom - rc.top;

    RECT rcWindow = { 0 };
    GetWindowRect(hwnd, &rcWindow);
    MapWindowPoints(HWND_DESKTOP, GetParent(hwnd), (LPPOINT)&rcWindow, 2);
    int x = rcWindow.left;
    int y = rcWindow.top;
    int width = rcWindow.right - rcWindow.left;
    int height = rcWindow.bottom - rcWindow.top;

    UINT currentDPI = 0;
    DPIAware::GetScreenDPIForWindow(m_hwnd, currentDPI);
    float scale = (float)currentDPI / m_initialDPI;

    switch (id)
    {
    case IDC_EDIT_SEARCHFOR:
    case IDC_EDIT_REPLACEWITH:
        width = mainWindowWidth - static_cast<int>(m_itemsPositioning.searchReplaceWidthDiff * scale);
        break;
    case IDC_PREVIEWGROUP:
        height = mainWindowHeight - static_cast<int>(m_itemsPositioning.previewGroupHeightDiff * scale);
    case IDC_SEARCHREPLACEGROUP:
    case IDC_OPTIONSGROUP:
        width = mainWindowWidth - static_cast<int>(m_itemsPositioning.groupsWidthDiff * scale);
        break;
    case IDC_LIST_PREVIEW:
        width = mainWindowWidth - static_cast<int>(m_itemsPositioning.listPreviewWidthDiff * scale);
        height = mainWindowHeight - static_cast<int>(m_itemsPositioning.listPreviewHeightDiff * scale);
        break;
    case IDC_STATUS_MESSAGE_SELECTED:
        y = mainWindowHeight - static_cast<int>(m_itemsPositioning.statusMessageSelectedYDiff * scale);
        break;
    case IDC_STATUS_MESSAGE_RENAMING:
        y = mainWindowHeight - static_cast<int>(m_itemsPositioning.statusMessageRenamingYDiff * scale);
        break;
    case ID_RENAME:
        x = mainWindowWidth - static_cast<int>(m_itemsPositioning.renameButtonXDiff * scale);
        y = mainWindowHeight - static_cast<int>(m_itemsPositioning.renameButtonYDiff * scale);
        break;
    case ID_ABOUT:
        x = mainWindowWidth - static_cast<int>(m_itemsPositioning.helpButtonXDiff * scale);
        y = mainWindowHeight - static_cast<int>(m_itemsPositioning.helpButtonYDiff * scale);
        break;
    case IDCANCEL:
        x = mainWindowWidth - static_cast<int>(m_itemsPositioning.cancelButtonXDiff * scale);
        y = mainWindowHeight - static_cast<int>(m_itemsPositioning.cancelButtonYDiff * scale);
        break;
    }

    SetWindowPos(hwnd, NULL, x, y, width, height, flags);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
}

void CPowerRenameUI::_OnSearchReplaceChanged()
{
    // Pass updated search and replace terms to the IPowerRenameRegEx handler
    CComPtr<IPowerRenameRegEx> spRegEx;
    if (m_spsrm && SUCCEEDED(m_spsrm->GetRenameRegEx(&spRegEx)))
    {
        wchar_t buffer[CSettings::MAX_INPUT_STRING_LEN];
        buffer[0] = L'\0';
        GetDlgItemText(m_hwnd, IDC_EDIT_SEARCHFOR, buffer, ARRAYSIZE(buffer));
        spRegEx->PutSearchTerm(buffer);

        buffer[0] = L'\0';
        GetDlgItemText(m_hwnd, IDC_EDIT_REPLACEWITH, buffer, ARRAYSIZE(buffer));
        spRegEx->PutReplaceTerm(buffer);
    }
}

DWORD CPowerRenameUI::_GetFlagsFromCheckboxes()
{
    DWORD flags = 0;
    for (int i = 0; i < ARRAYSIZE(g_flagCheckboxMap); i++)
    {
        if (Button_GetCheck(GetDlgItem(m_hwnd, g_flagCheckboxMap[i].id)) == BST_CHECKED)
        {
            flags |= g_flagCheckboxMap[i].flag;
        }
    }

    // Ensure we update flags
    if (m_spsrm)
    {
        m_spsrm->PutFlags(flags);
    }

    return flags;
}

void CPowerRenameUI::_SetCheckboxesFromFlags(_In_ DWORD flags)
{
    for (int i = 0; i < ARRAYSIZE(g_flagCheckboxMap); i++)
    {
        Button_SetCheck(GetDlgItem(m_hwnd, g_flagCheckboxMap[i].id), flags & g_flagCheckboxMap[i].flag);
    }
}

void CPowerRenameUI::_ValidateFlagCheckbox(_In_ DWORD checkBoxId)
{
    if (checkBoxId == IDC_TRANSFORM_UPPERCASE)
    {
        if (Button_GetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_UPPERCASE)) == BST_CHECKED)
        {
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_LOWERCASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_TITLECASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED), FALSE);
        }
    }
    else if (checkBoxId == IDC_TRANSFORM_LOWERCASE)
    {
        if (Button_GetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_LOWERCASE)) == BST_CHECKED)
        {
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_UPPERCASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_TITLECASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED), FALSE);
        }
    }
    else if (checkBoxId == IDC_TRANSFORM_TITLECASE)
    {
        if (Button_GetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_TITLECASE)) == BST_CHECKED)
        {
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_UPPERCASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_LOWERCASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED), FALSE);
        }
    }
    else if (checkBoxId == IDC_TRANSFORM_CAPITALIZED) 
    {
        if (Button_GetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED)) == BST_CHECKED)
        {
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_UPPERCASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_LOWERCASE), FALSE);
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_TITLECASE), FALSE);
        }
    }
    else if (checkBoxId == IDC_CHECK_NAMEONLY)
    {
        if (Button_GetCheck(GetDlgItem(m_hwnd, IDC_CHECK_NAMEONLY)) == BST_CHECKED)
        {
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_CHECK_EXTENSIONONLY), FALSE);
        }
    }
    else if (checkBoxId == IDC_CHECK_EXTENSIONONLY)
    {
        if (Button_GetCheck(GetDlgItem(m_hwnd, IDC_CHECK_EXTENSIONONLY)) == BST_CHECKED)
        {
            Button_SetCheck(GetDlgItem(m_hwnd, IDC_CHECK_NAMEONLY), FALSE);
        }
    }
}

void CPowerRenameUI::_UpdateCounts()
{
    // This method is CPU intensive.  We disable it during certain operations
    // for performance reasons.
    if (m_disableCountUpdate)
    {
        return;
    }

    UINT selectedCount = 0;
    UINT renamingCount = 0;
    if (m_spsrm)
    {
        m_spsrm->GetSelectedItemCount(&selectedCount);
        m_spsrm->GetRenameItemCount(&renamingCount);
    }

    if (m_selectedCount != selectedCount ||
        m_renamingCount != renamingCount)
    {
        m_selectedCount = selectedCount;
        m_renamingCount = renamingCount;

        // Update selected and rename count label
        wchar_t countsLabelFormatSelected[100] = { 0 };
        wchar_t countsLabelFormatRenaming[100] = { 0 };
        LoadString(g_hInst, IDS_COUNTSLABELSELECTEDFMT, countsLabelFormatSelected, ARRAYSIZE(countsLabelFormatSelected));
        LoadString(g_hInst, IDS_COUNTSLABELRENAMINGFMT, countsLabelFormatRenaming, ARRAYSIZE(countsLabelFormatRenaming));

        wchar_t countsLabelSelected[100] = { 0 };
        wchar_t countsLabelRenaming[100] = { 0 };
        StringCchPrintf(countsLabelSelected, ARRAYSIZE(countsLabelSelected), countsLabelFormatSelected, selectedCount);
        StringCchPrintf(countsLabelRenaming, ARRAYSIZE(countsLabelRenaming), countsLabelFormatRenaming, renamingCount);
        SetDlgItemText(m_hwnd, IDC_STATUS_MESSAGE_SELECTED, countsLabelSelected);
        SetDlgItemText(m_hwnd, IDC_STATUS_MESSAGE_RENAMING, countsLabelRenaming);

        // Update Rename button state
        EnableWindow(GetDlgItem(m_hwnd, ID_RENAME), (renamingCount > 0));
    }
}

void CPowerRenameUI::_CollectItemPosition(_In_ DWORD id)
{
    HWND hwnd = GetDlgItem(m_hwnd, id);
    RECT rcWindow = { 0 };
    GetWindowRect(hwnd, &rcWindow);

    MapWindowPoints(HWND_DESKTOP, GetParent(hwnd), (LPPOINT)&rcWindow, 2);
    int itemWidth = rcWindow.right - rcWindow.left;
    int itemHeight = rcWindow.bottom - rcWindow.top;

    switch (id)
    {
    case IDC_EDIT_SEARCHFOR:
        /* IDC_EDIT_REPLACEWITH uses same value*/
        m_itemsPositioning.searchReplaceWidthDiff = m_initialWidth - itemWidth;
        break;
    case IDC_PREVIEWGROUP:
        m_itemsPositioning.previewGroupHeightDiff = m_initialHeight - itemHeight;
        break;
    case IDC_SEARCHREPLACEGROUP:
        /* IDC_OPTIONSGROUP uses same value */
        m_itemsPositioning.groupsWidthDiff = m_initialWidth - itemWidth;
        break;
    case IDC_LIST_PREVIEW:
        m_itemsPositioning.listPreviewWidthDiff = m_initialWidth - itemWidth;
        m_itemsPositioning.listPreviewHeightDiff = m_initialHeight - itemHeight;
        break;
    case IDC_STATUS_MESSAGE_SELECTED:
        m_itemsPositioning.statusMessageSelectedYDiff = m_initialHeight - rcWindow.top;
        break;
    case IDC_STATUS_MESSAGE_RENAMING:
        m_itemsPositioning.statusMessageRenamingYDiff = m_initialHeight - rcWindow.top;
        break;
    case ID_RENAME:
        m_itemsPositioning.renameButtonXDiff = m_initialWidth - rcWindow.left;
        m_itemsPositioning.renameButtonYDiff = m_initialHeight - rcWindow.top;
        break;
    case ID_ABOUT:
        m_itemsPositioning.helpButtonXDiff = m_initialWidth - rcWindow.left;
        m_itemsPositioning.helpButtonYDiff = m_initialHeight - rcWindow.top;
        break;
    case IDCANCEL:
        m_itemsPositioning.cancelButtonXDiff = m_initialWidth - rcWindow.left;
        m_itemsPositioning.cancelButtonYDiff = m_initialHeight - rcWindow.top;
        break;
    }
}

void CPowerRenameListView::Init(_In_ HWND hwndLV)
{
    if (hwndLV)
    {
        m_hwndLV = hwndLV;

        EnableWindow(m_hwndLV, TRUE);

        // Set the standard styles
        DWORD dwLVStyle = (DWORD)GetWindowLongPtr(m_hwndLV, GWL_STYLE);
        dwLVStyle |= LVS_ALIGNLEFT | LVS_REPORT | LVS_SHAREIMAGELISTS | LVS_SINGLESEL;
        SetWindowLongPtr(m_hwndLV, GWL_STYLE, dwLVStyle);

        // Set the extended view styles
        ListView_SetExtendedListViewStyle(m_hwndLV, LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER | LVS_EX_AUTOSIZECOLUMNS);

        // Get the system image lists.  Our list view is setup to not destroy
        // these since the image list belongs to the entire explorer process
        HIMAGELIST himlLarge;
        HIMAGELIST himlSmall;
        if (Shell_GetImageLists(&himlLarge, &himlSmall))
        {
            ListView_SetImageList(m_hwndLV, himlSmall, LVSIL_SMALL);
            ListView_SetImageList(m_hwndLV, himlLarge, LVSIL_NORMAL);
        }

        _UpdateColumns();
    }
}

void CPowerRenameListView::ToggleAll(_In_ IPowerRenameManager* psrm, _In_ bool selected)
{
    if (m_hwndLV)
    {
        UINT visibleItemCount = 0, itemCount = 0;
        psrm->GetItemCount(&itemCount);
        for (UINT i = 0; i < itemCount; i++)
        {
            CComPtr<IPowerRenameItem> spItem;
            if (SUCCEEDED(psrm->GetItemByIndex(i, &spItem)))
            {
                spItem->PutSelected(selected);
            }
        }

        psrm->GetVisibleItemCount(&visibleItemCount);
        SetItemCount(visibleItemCount);
        RedrawItems(0, visibleItemCount);
    }
}

void CPowerRenameListView::ToggleItem(_In_ IPowerRenameManager* psrm, _In_ int item)
{
    CComPtr<IPowerRenameItem> spItem;
    if (SUCCEEDED(psrm->GetVisibleItemByIndex(item, &spItem)))
    {
        bool selected = false;
        spItem->GetSelected(&selected);
        spItem->PutSelected(!selected);

        UINT visibleItemCount = 0;
        psrm->GetVisibleItemCount(&visibleItemCount);
        SetItemCount(visibleItemCount);
        RedrawItems(0, visibleItemCount);
    }
}

void CPowerRenameListView::OnKeyDown(_In_ IPowerRenameManager* psrm, _In_ LV_KEYDOWN* lvKeyDown)
{
    if (lvKeyDown->wVKey == VK_SPACE)
    {
        int selectionMark = ListView_GetSelectionMark(m_hwndLV);
        if (selectionMark != -1)
        {
            ToggleItem(psrm, selectionMark);
        }
    }
}

void CPowerRenameListView::OnClickList(_In_ IPowerRenameManager* psrm, NM_LISTVIEW* pnmListView)
{
    LVHITTESTINFO hitinfo;
    //Copy click point
    hitinfo.pt = pnmListView->ptAction;

    //Make the hit test...
    int item = ListView_HitTest(m_hwndLV, &hitinfo);
    if (item != -1)
    {
        if ((hitinfo.flags & LVHT_ONITEM) != 0)
        {
            ToggleItem(psrm, item);
        }
    }
}

void CPowerRenameListView::UpdateItemCheckState(_In_ IPowerRenameManager* psrm, _In_ int iItem)
{
    if (psrm && m_hwndLV && (iItem > -1))
    {
        CComPtr<IPowerRenameItem> spItem;
        if (SUCCEEDED(psrm->GetVisibleItemByIndex(iItem, &spItem)))
        {
            bool checked = ListView_GetCheckState(m_hwndLV, iItem);
            spItem->PutSelected(checked);

            UINT uSelected = (checked) ? LVIS_SELECTED : 0;
            ListView_SetItemState(m_hwndLV, iItem, uSelected, LVIS_SELECTED);

            // Update the rename column if necessary
            UINT visibleItemCount = 0;
            psrm->GetVisibleItemCount(&visibleItemCount);
            SetItemCount(visibleItemCount);
            RedrawItems(0, visibleItemCount);
        }

        // Get the total number of list items and compare it to what is selected
        // We need to update the column checkbox if all items are selected or if
        // not all of the items are selected.
        bool checkHeader = (ListView_GetSelectedCount(m_hwndLV) == ListView_GetItemCount(m_hwndLV));
        _UpdateHeaderCheckState(checkHeader);
    }
}

#define COL_ORIGINAL_NAME 0
#define COL_NEW_NAME 1

void CPowerRenameListView::GetDisplayInfo(_In_ IPowerRenameManager* psrm, _Inout_ LV_DISPINFO* plvdi)
{
    UINT count = 0;
    psrm->GetVisibleItemCount(&count);
    if (plvdi->item.iItem < 0 || plvdi->item.iItem > static_cast<int>(count))
    {
        // Invalid index
        return;
    }

    CComPtr<IPowerRenameItem> renameItem;
    if (SUCCEEDED(psrm->GetVisibleItemByIndex((int)plvdi->item.iItem, &renameItem)))
    {
        if (plvdi->item.mask & LVIF_IMAGE)
        {
            renameItem->GetIconIndex(&plvdi->item.iImage);
        }

        if (plvdi->item.mask & LVIF_STATE)
        {
            plvdi->item.stateMask = LVIS_STATEIMAGEMASK;

            bool isSelected = false;
            renameItem->GetSelected(&isSelected);
            if (isSelected)
            {
                // Turn check box on
                plvdi->item.state = INDEXTOSTATEIMAGEMASK(2);
            }
            else
            {
                // Turn check box off
                plvdi->item.state = INDEXTOSTATEIMAGEMASK(1);
            }
        }

        if (plvdi->item.mask & LVIF_PARAM)
        {
            int id = 0;
            renameItem->GetId(&id);
            plvdi->item.lParam = static_cast<LPARAM>(id);
        }

        if (plvdi->item.mask & LVIF_INDENT)
        {
            UINT depth = 0;
            renameItem->GetDepth(&depth);
            plvdi->item.iIndent = static_cast<int>(depth);
        }

        if (plvdi->item.mask & LVIF_TEXT)
        {
            PWSTR subItemText = nullptr;
            if (plvdi->item.iSubItem == COL_ORIGINAL_NAME)
            {
                renameItem->GetOriginalName(&subItemText);
            }
            else if (plvdi->item.iSubItem == COL_NEW_NAME)
            {
                DWORD flags = 0;
                psrm->GetFlags(&flags);
                bool shouldRename = false;
                if (SUCCEEDED(renameItem->ShouldRenameItem(flags, &shouldRename)) && shouldRename)
                {
                    renameItem->GetNewName(&subItemText);
                }
            }

            StringCchCopy(plvdi->item.pszText, plvdi->item.cchTextMax, subItemText ? subItemText : L"");
            CoTaskMemFree(subItemText);
            subItemText = nullptr;
        }
    }
}

void CPowerRenameListView::OnSize()
{
    RECT rc = { 0 };
    GetClientRect(m_hwndLV, &rc);
    ListView_SetColumnWidth(m_hwndLV, 0, RECT_WIDTH(rc) / 2);
    ListView_SetColumnWidth(m_hwndLV, 1, RECT_WIDTH(rc) / 2);
}

void CPowerRenameListView::RedrawItems(_In_ int first, _In_ int last)
{
    ListView_RedrawItems(m_hwndLV, first, last);
}

void CPowerRenameListView::SetItemCount(_In_ UINT itemCount)
{
    if (m_itemCount != itemCount)
    {
        m_itemCount = itemCount;
        ListView_SetItemCount(m_hwndLV, itemCount);
    }
}

void CPowerRenameListView::_UpdateColumns()
{
    if (m_hwndLV)
    {
        // And the list view columns
        int iInsertPoint = 0;

        LV_COLUMN lvc = { 0 };
        lvc.mask = LVCF_FMT | LVCF_ORDER | LVCF_WIDTH | LVCF_TEXT;
        lvc.fmt = LVCFMT_LEFT;
        lvc.iOrder = iInsertPoint;

        wchar_t buffer[64] = { 0 };
        LoadString(g_hInst, IDS_ORIGINAL, buffer, ARRAYSIZE(buffer));
        lvc.pszText = buffer;

        ListView_InsertColumn(m_hwndLV, iInsertPoint, &lvc);

        iInsertPoint++;

        lvc.iOrder = iInsertPoint;
        LoadString(g_hInst, IDS_RENAMED, buffer, ARRAYSIZE(buffer));
        lvc.pszText = buffer;

        ListView_InsertColumn(m_hwndLV, iInsertPoint, &lvc);

        // Get a handle to the header of the columns
        HWND hwndHeader = ListView_GetHeader(m_hwndLV);

        if (hwndHeader)
        {
            // Update the header style to allow checkboxes
            DWORD dwHeaderStyle = (DWORD)GetWindowLongPtr(hwndHeader, GWL_STYLE);
            dwHeaderStyle |= HDS_CHECKBOXES;
            SetWindowLongPtr(hwndHeader, GWL_STYLE, dwHeaderStyle);

            _UpdateHeaderCheckState(TRUE);
        }

        _UpdateColumnSizes();
    }
}

void CPowerRenameListView::_UpdateColumnSizes()
{
    if (m_hwndLV)
    {
        RECT rc;
        GetClientRect(m_hwndLV, &rc);

        ListView_SetColumnWidth(m_hwndLV, 0, (rc.right - rc.left) / 2);
        ListView_SetColumnWidth(m_hwndLV, 1, (rc.right - rc.left) / 2);
    }
}

void CPowerRenameListView::_UpdateHeaderCheckState(_In_ bool check)
{
    // Get a handle to the header of the columns
    HWND hwndHeader = ListView_GetHeader(m_hwndLV);
    if (hwndHeader)
    {
        wchar_t buffer[MAX_PATH] = { 0 };
        buffer[0] = L'\0';

        // Retrieve the existing header first so we
        // don't trash the text already there
        HDITEM hdi = { 0 };
        hdi.mask = HDI_FORMAT | HDI_TEXT;
        hdi.pszText = buffer;
        hdi.cchTextMax = ARRAYSIZE(buffer);

        Header_GetItem(hwndHeader, 0, &hdi);

        // Set the first column to contain a checkbox
        hdi.fmt |= HDF_CHECKBOX;
        hdi.fmt |= (check) ? HDF_CHECKED : 0;

        Header_SetItem(hwndHeader, 0, &hdi);
    }
}

void CPowerRenameListView::_UpdateHeaderFilterState(_In_ DWORD filter)
{
    // Get a handle to the header of the columns
    HWND hwndHeader = ListView_GetHeader(m_hwndLV);
    if (hwndHeader)
    {
        wchar_t bufferOriginal[MAX_PATH] = { 0 };
        bufferOriginal[0] = L'\0';

        // Retrieve the existing header first so we
        // don't trash the text already there
        HDITEM hdiOriginal = { 0 };
        hdiOriginal.mask = HDI_FORMAT | HDI_TEXT;
        hdiOriginal.pszText = bufferOriginal;
        hdiOriginal.cchTextMax = ARRAYSIZE(bufferOriginal);

        Header_GetItem(hwndHeader, 0, &hdiOriginal);

        if (filter == PowerRenameFilters::Selected || filter == PowerRenameFilters::FlagsApplicable)
        {
            hdiOriginal.fmt |= HDF_SORTDOWN;
        }
        else
        {
            hdiOriginal.fmt &= ~HDF_SORTDOWN;
        }

        Header_SetItem(hwndHeader, 0, &hdiOriginal);

        wchar_t bufferRename[MAX_PATH] = { 0 };
        bufferRename[0] = L'\0';

        // Retrieve the existing header first so we
        // don't trash the text already there
        HDITEM hdiRename = { 0 };
        hdiRename.mask = HDI_FORMAT | HDI_TEXT;
        hdiRename.pszText = bufferRename;
        hdiRename.cchTextMax = ARRAYSIZE(bufferRename);

        Header_GetItem(hwndHeader, 1, &hdiRename);

        if (filter == PowerRenameFilters::ShouldRename)
        {
            hdiRename.fmt |= HDF_SORTDOWN;
        }
        else
        {
            hdiRename.fmt &= ~HDF_SORTDOWN;
        }

        Header_SetItem(hwndHeader, 1, &hdiRename);
    }
}

void CPowerRenameListView::OnColumnClick(_In_ IPowerRenameManager* psrm, _In_ int columnNumber)
{
    DWORD filter = PowerRenameFilters::None;
    psrm->SwitchFilter(columnNumber);
    UINT visibleItemCount = 0;
    psrm->GetVisibleItemCount(&visibleItemCount);
    SetItemCount(visibleItemCount);
    RedrawItems(0, visibleItemCount);

    psrm->GetFilter(&filter);
    _UpdateHeaderFilterState(filter);
}

IFACEMETHODIMP CPowerRenameProgressUI::QueryInterface(__in REFIID riid, __deref_out void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CPowerRenameProgressUI, IUnknown),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG)
CPowerRenameProgressUI::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG)
CPowerRenameProgressUI::Release()
{
    long refCount = InterlockedDecrement(&m_refCount);
    if (refCount == 0)
    {
        delete this;
    }
    return refCount;
}

#define TIMERID_CHECKCANCELED 101
#define CANCEL_CHECK_INTERVAL 500

HRESULT CPowerRenameProgressUI::Start()
{
    _Cleanup();
    m_canceled = false;
    AddRef();
    m_workerThreadHandle = CreateThread(nullptr, 0, s_workerThread, this, 0, nullptr);
    if (!m_workerThreadHandle)
    {
        Release();
    }
    return (m_workerThreadHandle) ? S_OK : E_FAIL;
}

DWORD WINAPI CPowerRenameProgressUI::s_workerThread(_In_ void* pv)
{
    if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
    {
        CPowerRenameProgressUI* pThis = reinterpret_cast<CPowerRenameProgressUI*>(pv);
        if (pThis)
        {
            HWND hwndMessage = CreateMsgWindow(g_hInst, s_msgWndProc, pThis);

            SetTimer(hwndMessage, TIMERID_CHECKCANCELED, CANCEL_CHECK_INTERVAL, nullptr);

            CComPtr<IProgressDialog> sppd;
            if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&sppd))))
            {
                pThis->m_sppd = sppd;
                wchar_t buff[100] = { 0 };
                LoadString(g_hInst, IDS_LOADING, buff, ARRAYSIZE(buff));
                sppd->SetLine(1, buff, FALSE, NULL);
                LoadString(g_hInst, IDS_LOADING_MSG, buff, ARRAYSIZE(buff));
                sppd->SetLine(2, buff, FALSE, NULL);
                LoadString(g_hInst, IDS_APP_TITLE, buff, ARRAYSIZE(buff));
                sppd->SetTitle(buff);
                SetTimer(hwndMessage, TIMERID_CHECKCANCELED, CANCEL_CHECK_INTERVAL, nullptr);
                sppd->StartProgressDialog(NULL, NULL, PROGDLG_MARQUEEPROGRESS, NULL);
            }

            while (pThis->m_sppd && !sppd->HasUserCancelled())
            {
                MSG msg;
                while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            // Ensure dialog is stopped
            sppd->StopProgressDialog();

            KillTimer(hwndMessage, TIMERID_CHECKCANCELED);
            DestroyWindow(hwndMessage);

            pThis->Release();
        }

        CoUninitialize();
    }

    return S_OK;
}

HRESULT CPowerRenameProgressUI::Stop()
{
    _Cleanup();
    return S_OK;
}

void CPowerRenameProgressUI::_Cleanup()
{
    if (m_sppd)
    {
        m_sppd->StopProgressDialog();
        m_sppd = nullptr;
    }

    if (m_workerThreadHandle)
    {
        // Wait for up to 5 seconds for worker thread to finish
        WaitForSingleObject(m_workerThreadHandle, 5000);
        CloseHandle(m_workerThreadHandle);
        m_workerThreadHandle = nullptr;
    }
    
}

void CPowerRenameProgressUI::_UpdateCancelState()
{
    m_canceled = (m_sppd && m_sppd->HasUserCancelled());
}

LRESULT CALLBACK CPowerRenameProgressUI::s_msgWndProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    LRESULT lRes = 0;

    CPowerRenameProgressUI* pThis = (CPowerRenameProgressUI*)GetWindowLongPtr(hwnd, 0);
    if (pThis != nullptr)
    {
        lRes = pThis->_WndProc(hwnd, uMsg, wParam, lParam);
        if (uMsg == WM_NCDESTROY)
        {
            SetWindowLongPtr(hwnd, 0, NULL);
        }
    }
    else
    {
        lRes = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lRes;
}

LRESULT CPowerRenameProgressUI::_WndProc(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    LRESULT lRes = 0;

    AddRef();

    switch (msg)
    {
    case WM_TIMER:
        if (wParam == TIMERID_CHECKCANCELED)
        {
            _UpdateCancelState();
        }
        break;

    case WM_DESTROY:
        _UpdateCancelState();
        KillTimer(hwnd, TIMERID_CHECKCANCELED);
        break;

    default:
        lRes = DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }

    Release();

    return lRes;
}