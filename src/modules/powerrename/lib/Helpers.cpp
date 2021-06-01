#include "pch.h"
#include "Helpers.h"
#include <regex>
#include <ShlGuid.h>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

HRESULT GetTrimmedFileName(_Out_ PWSTR result, UINT cchMax, _In_ PCWSTR source)
{
    HRESULT hr = E_INVALIDARG;
    if (source)
    {
        PWSTR newName = nullptr;
        hr = SHStrDup(source, &newName);
        if (SUCCEEDED(hr))
        {
            size_t firstValidIndex = 0, lastValidIndex = wcslen(newName) - 1;
            while (firstValidIndex <= lastValidIndex && iswspace(newName[firstValidIndex]))
            {
                firstValidIndex++;
            }
            while (firstValidIndex <= lastValidIndex && (iswspace(newName[lastValidIndex]) || newName[lastValidIndex] == L'.'))
            {
                lastValidIndex--;
            }
            newName[lastValidIndex + 1] = '\0';

            hr = StringCchCopy(result, cchMax, newName + firstValidIndex);
        }
        CoTaskMemFree(newName);
    }

    return hr;
}

HRESULT GetTransformedFileName(_Out_ PWSTR result, UINT cchMax, _In_ PCWSTR source, DWORD flags)
{
    std::locale::global(std::locale(""));
    HRESULT hr = E_INVALIDARG;
    if (source && flags)
    {
        if (flags & Uppercase)
        {
            if (flags & NameOnly)
            {
                std::wstring stem = fs::path(source).stem().wstring();
                std::transform(stem.begin(), stem.end(), stem.begin(), ::towupper);
                hr = StringCchPrintf(result, cchMax, L"%s%s", stem.c_str(), fs::path(source).extension().c_str());
            }
            else if (flags & ExtensionOnly)
            {
                std::wstring extension = fs::path(source).extension().wstring();
                if (!extension.empty())
                {
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::towupper);
                    hr = StringCchPrintf(result, cchMax, L"%s%s", fs::path(source).stem().c_str(), extension.c_str());
                }
                else
                {
                    hr = StringCchCopy(result, cchMax, source);
                    if (SUCCEEDED(hr))
                    {
                        std::transform(result, result + wcslen(result), result, ::towupper);
                    }
                }
            }
            else
            {
                hr = StringCchCopy(result, cchMax, source);
                if (SUCCEEDED(hr))
                {
                    std::transform(result, result + wcslen(result), result, ::towupper);
                }
            }
        }
        else if (flags & Lowercase)
        {
            if (flags & NameOnly)
            {
                std::wstring stem = fs::path(source).stem().wstring();
                std::transform(stem.begin(), stem.end(), stem.begin(), ::towlower);
                hr = StringCchPrintf(result, cchMax, L"%s%s", stem.c_str(), fs::path(source).extension().c_str());
            }
            else if (flags & ExtensionOnly)
            {
                std::wstring extension = fs::path(source).extension().wstring();
                if (!extension.empty())
                {
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
                    hr = StringCchPrintf(result, cchMax, L"%s%s", fs::path(source).stem().c_str(), extension.c_str());
                }
                else
                {
                    hr = StringCchCopy(result, cchMax, source);
                    if (SUCCEEDED(hr))
                    {
                        std::transform(result, result + wcslen(result), result, ::towlower);
                    }
                }
            }
            else
            {
                hr = StringCchCopy(result, cchMax, source);
                if (SUCCEEDED(hr))
                {
                    std::transform(result, result + wcslen(result), result, ::towlower);
                }
            }
        }
        else if (flags & Titlecase)
        {
            if (!(flags & ExtensionOnly))
            {
                std::vector<std::wstring> exceptions = { L"a", L"an", L"to", L"the", L"at", L"by", L"for", L"in", L"of", L"on", L"up", L"and", L"as", L"but", L"or", L"nor" };
                std::wstring stem = fs::path(source).stem().wstring();
                std::wstring extension = fs::path(source).extension().wstring();

                size_t stemLength = stem.length();
                bool isFirstWord = true;

                while (stemLength > 0 && (iswspace(stem[stemLength - 1]) || iswpunct(stem[stemLength - 1])))
                {
                    stemLength--;
                }

                for (size_t i = 0; i < stemLength; i++)
                {
                    if (!i || iswspace(stem[i - 1]) || iswpunct(stem[i - 1]))
                    {
                        if (iswspace(stem[i]) || iswpunct(stem[i]))
                        {
                            continue;
                        }
                        size_t wordLength = 0;
                        while (i + wordLength < stemLength && !iswspace(stem[i + wordLength]) && !iswpunct(stem[i + wordLength]))
                        {
                            wordLength++;
                        }
                        if (isFirstWord || i + wordLength == stemLength || std::find(exceptions.begin(), exceptions.end(), stem.substr(i, wordLength)) == exceptions.end())
                        {
                            stem[i] = towupper(stem[i]);
                            isFirstWord = false;
                        }
                        else
                        {
                            stem[i] = towlower(stem[i]);
                        }
                    }
                    else
                    {
                        stem[i] = towlower(stem[i]);
                    }
                }
                hr = StringCchPrintf(result, cchMax, L"%s%s", stem.c_str(), extension.c_str());
            }
            else
            {
                hr = StringCchCopy(result, cchMax, source);
            }
        } 
        else if (flags & Capitalized)
        {
            if (!(flags & ExtensionOnly))
            {
                std::wstring stem = fs::path(source).stem().wstring();
                std::wstring extension = fs::path(source).extension().wstring();

                size_t stemLength = stem.length();

                while (stemLength > 0 && (iswspace(stem[stemLength - 1]) || iswpunct(stem[stemLength - 1])))
                {
                    stemLength--;
                }

                for (size_t i = 0; i < stemLength; i++)
                {
                    if (!i || iswspace(stem[i - 1]) || iswpunct(stem[i - 1]))
                    {
                        if (iswspace(stem[i]) || iswpunct(stem[i]))
                        {
                            continue;
                        }
                        stem[i] = towupper(stem[i]);
                    }
                    else
                    {
                        stem[i] = towlower(stem[i]);
                    }
                }
                hr = StringCchPrintf(result, cchMax, L"%s%s", stem.c_str(), extension.c_str());
            }
            else
            {
                hr = StringCchCopy(result, cchMax, source);
            }
        }
        else
        {
            hr = StringCchCopy(result, cchMax, source);
        }
    }

    return hr;
}

bool isFileTimeUsed(_In_ PCWSTR source) 
{
    bool used = false;
    std::wstring patterns[] = { L"(([^\\$]|^)(\\$\\$)*)\\$Y", L"(([^\\$]|^)(\\$\\$)*)\\$M", L"(([^\\$]|^)(\\$\\$)*)\\$D", 
        L"(([^\\$]|^)(\\$\\$)*)\\$h", L"(([^\\$]|^)(\\$\\$)*)\\$m", L"(([^\\$]|^)(\\$\\$)*)\\$s", L"(([^\\$]|^)(\\$\\$)*)\\$f" };
    size_t patternsLength = ARRAYSIZE(patterns);
    for (size_t i = 0; !used && i < patternsLength; i++)
    {
        if (std::regex_search(source, std::wregex(patterns[i])))
        {
            used = true;
        }
    }
    return used;
}

HRESULT GetDatedFileName(_Out_ PWSTR result, UINT cchMax, _In_ PCWSTR source, SYSTEMTIME fileTime)
{
    std::locale::global(std::locale(""));
    HRESULT hr = E_INVALIDARG;     
    if (source && wcslen(source) > 0)
    {
        std::wstring res(source);
        wchar_t replaceTerm[MAX_PATH] = { 0 };
        wchar_t formattedDate[MAX_PATH] = { 0 };

        wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
        if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) == 0)
        {
            StringCchCopy(localeName, LOCALE_NAME_MAX_LENGTH, L"en_US");
        }

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%04d"), L"$01", fileTime.wYear);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$YYYY"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%02d"), L"$01", (fileTime.wYear % 100));
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$YY"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%d"), L"$01", (fileTime.wYear % 10));
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$Y"), replaceTerm);
        
        GetDateFormatEx(localeName, NULL, &fileTime, L"MMMM", formattedDate, MAX_PATH, NULL);
        formattedDate[0] = towupper(formattedDate[0]);
        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%s"), L"$01", formattedDate);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$MMMM"), replaceTerm);
        
        GetDateFormatEx(localeName, NULL, &fileTime, L"MMM", formattedDate, MAX_PATH, NULL);
        formattedDate[0] = towupper(formattedDate[0]);
        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%s"), L"$01", formattedDate);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$MMM"), replaceTerm);
                
        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%02d"), L"$01", fileTime.wMonth);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$MM"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%d"), L"$01", fileTime.wMonth);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$M"), replaceTerm);

        GetDateFormatEx(localeName, NULL, &fileTime, L"dddd", formattedDate, MAX_PATH, NULL);
        formattedDate[0] = towupper(formattedDate[0]);
        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%s"), L"$01", formattedDate);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$DDDD"), replaceTerm);
        
        GetDateFormatEx(localeName, NULL, &fileTime, L"ddd", formattedDate, MAX_PATH, NULL);
        formattedDate[0] = towupper(formattedDate[0]);
        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%s"), L"$01", formattedDate);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$DDD"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%02d"), L"$01", fileTime.wDay);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$DD"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%d"), L"$01", fileTime.wDay);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$D"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%02d"), L"$01", fileTime.wHour);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$hh"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%d"), L"$01", fileTime.wHour);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$h"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%02d"), L"$01", fileTime.wMinute);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$mm"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%d"), L"$01", fileTime.wMinute);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$m"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%02d"), L"$01", fileTime.wSecond);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$ss"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%d"), L"$01", fileTime.wSecond);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$s"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%03d"), L"$01", fileTime.wMilliseconds);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$fff"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%02d"), L"$01", fileTime.wMilliseconds/10);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$ff"), replaceTerm);

        StringCchPrintf(replaceTerm, MAX_PATH, TEXT("%s%d"), L"$01", fileTime.wMilliseconds/100);
        res = regex_replace(res, std::wregex(L"(([^\\$]|^)(\\$\\$)*)\\$f"), replaceTerm);

        hr = StringCchCopy(result, cchMax, res.c_str());
    }

    return hr;
}

HRESULT GetShellItemArrayFromDataObject(_In_ IUnknown* dataSource, _COM_Outptr_ IShellItemArray** items)
{
    *items = nullptr;
    CComPtr<IDataObject> dataObj;
    HRESULT hr;
    if (SUCCEEDED(dataSource->QueryInterface(IID_PPV_ARGS(&dataObj))))
    {
        hr = SHCreateShellItemArrayFromDataObject(dataObj, IID_PPV_ARGS(items));
    }
    else
    {
        hr = dataSource->QueryInterface(IID_PPV_ARGS(items));
    }

    return hr;
}

BOOL GetEnumeratedFileName(__out_ecount(cchMax) PWSTR pszUniqueName, UINT cchMax, __in PCWSTR pszTemplate, __in_opt PCWSTR pszDir, unsigned long ulMinLong, __inout unsigned long* pulNumUsed)
{
    PWSTR pszName = nullptr;
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    int cchDir = 0;

    if (0 != cchMax && pszUniqueName)
    {
        *pszUniqueName = 0;
        if (pszDir)
        {
            hr = StringCchCopy(pszUniqueName, cchMax, pszDir);
            if (SUCCEEDED(hr))
            {
                hr = PathCchAddBackslashEx(pszUniqueName, cchMax, &pszName, nullptr);
                if (SUCCEEDED(hr))
                {
                    cchDir = lstrlen(pszDir);
                }
            }
        }
        else
        {
            cchDir = 0;
            pszName = pszUniqueName;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    int cchTmp = 0;
    int cchStem = 0;
    PCWSTR pszStem = nullptr;
    PCWSTR pszRest = nullptr;
    wchar_t szFormat[MAX_PATH] = { 0 };

    if (SUCCEEDED(hr))
    {
        pszStem = pszTemplate;

        pszRest = StrChr(pszTemplate, L'(');
        while (pszRest)
        {
            PCWSTR pszEndUniq = CharNext(pszRest);
            while (*pszEndUniq && *pszEndUniq >= L'0' && *pszEndUniq <= L'9')
            {
                pszEndUniq++;
            }

            if (*pszEndUniq == L')')
            {
                break;
            }

            pszRest = StrChr(CharNext(pszRest), L'(');
        }

        if (!pszRest)
        {
            pszRest = PathFindExtension(pszTemplate);
            cchStem = (int)(pszRest - pszTemplate);

            hr = StringCchCopy(szFormat, ARRAYSIZE(szFormat), L" (%lu)");
        }
        else
        {
            pszRest++;

            cchStem = (int)(pszRest - pszTemplate);

            while (*pszRest && *pszRest >= L'0' && *pszRest <= L'9')
            {
                pszRest++;
            }

            hr = StringCchCopy(szFormat, ARRAYSIZE(szFormat), L"%lu");
        }
    }

    unsigned long ulMax = 0;
    unsigned long ulMin = 0;
    if (SUCCEEDED(hr))
    {
        int cchFormat = lstrlen(szFormat);
        if (cchFormat < 3)
        {
            *pszUniqueName = L'\0';
            return FALSE;
        }
        ulMin = ulMinLong;
        cchTmp = cchMax - cchDir - cchStem - (cchFormat - 3);
        switch (cchTmp)
        {
        case 1:
            ulMax = 10;
            break;
        case 2:
            ulMax = 100;
            break;
        case 3:
            ulMax = 1000;
            break;
        case 4:
            ulMax = 10000;
            break;
        case 5:
            ulMax = 100000;
            break;
        default:
            if (cchTmp <= 0)
            {
                ulMax = ulMin;
            }
            else
            {
                ulMax = 1000000;
            }
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = StringCchCopyN(pszName, pszUniqueName + cchMax - pszName, pszStem, cchStem);
        if (SUCCEEDED(hr))
        {
            PWSTR pszDigit = pszName + cchStem;

            for (unsigned long ul = ulMin; ((ul < ulMax) && (!fRet)); ul++)
            {
                wchar_t szTemp[MAX_PATH] = { 0 };
                hr = StringCchPrintf(szTemp, ARRAYSIZE(szTemp), szFormat, ul);
                if (SUCCEEDED(hr))
                {
                    hr = StringCchCat(szTemp, ARRAYSIZE(szTemp), pszRest);
                    if (SUCCEEDED(hr))
                    {
                        hr = StringCchCopy(pszDigit, pszUniqueName + cchMax - pszDigit, szTemp);
                        if (SUCCEEDED(hr))
                        {
                            if (!PathFileExists(pszUniqueName))
                            {
                                (*pulNumUsed) = ul;
                                fRet = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!fRet)
    {
        *pszUniqueName = L'\0';
    }

    return fRet;
}

// Iterate through the data source and checks if at least 1 item has SFGAO_CANRENAME.
// We do not enumerate child items - only the items the user selected.
bool DataObjectContainsRenamableItem(_In_ IUnknown* dataSource)
{
    bool hasRenamable = false;
    CComPtr<IShellItemArray> spsia;
    if (SUCCEEDED(GetShellItemArrayFromDataObject(dataSource, &spsia)))
    {
        CComPtr<IEnumShellItems> spesi;
        if (SUCCEEDED(spsia->EnumItems(&spesi)))
        {
            ULONG celtFetched;
            CComPtr<IShellItem> spsi;
            while ((S_OK == spesi->Next(1, &spsi, &celtFetched)))
            {
                SFGAOF attrs;
                if (SUCCEEDED(spsi->GetAttributes(SFGAO_CANRENAME, &attrs)) &&
                    attrs & SFGAO_CANRENAME)
                {
                    hasRenamable = true;
                    break;
                }
            }
        }
    }
    return hasRenamable;
}

HWND CreateMsgWindow(_In_ HINSTANCE hInst, _In_ WNDPROC pfnWndProc, _In_ void* p)
{
    WNDCLASS wc = { 0 };
    PCWSTR wndClassName = L"MsgWindow";

    wc.lpfnWndProc = DefWindowProc;
    wc.cbWndExtra = sizeof(void*);
    wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = wndClassName;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, wndClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hInst, nullptr);
    if (hwnd)
    {
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)p);
        if (pfnWndProc)
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)pfnWndProc);
        }
    }

    return hwnd;
}
