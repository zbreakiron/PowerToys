#pragma once

#include "gdiplus.h"
#include <common/utils/string_utils.h>

namespace FancyZonesDataTypes
{
    struct DeviceIdData;
}

namespace FancyZonesUtils
{
    struct Rect
    {
        Rect() {}

        Rect(RECT rect) :
            m_rect(rect)
        {
        }

        Rect(RECT rect, UINT dpi) :
            m_rect(rect)
        {
            m_rect.right = m_rect.left + MulDiv(m_rect.right - m_rect.left, dpi, 96);
            m_rect.bottom = m_rect.top + MulDiv(m_rect.bottom - m_rect.top, dpi, 96);
        }

        int x() const { return m_rect.left; }
        int y() const { return m_rect.top; }
        int width() const { return m_rect.right - m_rect.left; }
        int height() const { return m_rect.bottom - m_rect.top; }
        int left() const { return m_rect.left; }
        int top() const { return m_rect.top; }
        int right() const { return m_rect.right; }
        int bottom() const { return m_rect.bottom; }
        int aspectRatio() const { return MulDiv(m_rect.bottom - m_rect.top, 100, m_rect.right - m_rect.left); }

    private:
        RECT m_rect{};
    };

    inline void MakeWindowTransparent(HWND window)
    {
        int const pos = -GetSystemMetrics(SM_CXVIRTUALSCREEN) - 8;
        if (wil::unique_hrgn hrgn{ CreateRectRgn(pos, 0, (pos + 1), 1) })
        {
            DWM_BLURBEHIND bh = { DWM_BB_ENABLE | DWM_BB_BLURREGION, TRUE, hrgn.get(), FALSE };
            DwmEnableBlurBehindWindow(window, &bh);
        }
    }

    inline void InitRGB(_Out_ RGBQUAD* quad, BYTE alpha, COLORREF color)
    {
        ZeroMemory(quad, sizeof(*quad));
        quad->rgbReserved = alpha;
        quad->rgbRed = GetRValue(color) * alpha / 255;
        quad->rgbGreen = GetGValue(color) * alpha / 255;
        quad->rgbBlue = GetBValue(color) * alpha / 255;
    }

    inline void FillRectARGB(wil::unique_hdc& hdc, RECT const* prcFill, BYTE alpha, COLORREF color, bool blendAlpha)
    {
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = 1;
        bi.bmiHeader.biHeight = 1;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        RECT fillRect;
        CopyRect(&fillRect, prcFill);

        RGBQUAD bitmapBits;
        InitRGB(&bitmapBits, alpha, color);
        StretchDIBits(
            hdc.get(),
            fillRect.left,
            fillRect.top,
            fillRect.right - fillRect.left,
            fillRect.bottom - fillRect.top,
            0,
            0,
            1,
            1,
            &bitmapBits,
            &bi,
            DIB_RGB_COLORS,
            SRCCOPY);
    }

    inline COLORREF HexToRGB(std::wstring_view hex, const COLORREF fallbackColor = RGB(255, 255, 255))
    {
        hex = left_trim<wchar_t>(trim<wchar_t>(hex), L"#");

        try
        {
            const long long tmp = std::stoll(hex.data(), nullptr, 16);
            const BYTE nR = static_cast<BYTE>((tmp & 0xFF0000) >> 16);
            const BYTE nG = static_cast<BYTE>((tmp & 0xFF00) >> 8);
            const BYTE nB = static_cast<BYTE>((tmp & 0xFF));
            return RGB(nR, nG, nB);
        }
        catch (const std::exception&)
        {
            return fallbackColor;
        }
    }

    inline BYTE OpacitySettingToAlpha(int opacity)
    {
        return static_cast<BYTE>(opacity * 2.55);
    }

    template<RECT MONITORINFO::*member>
    std::vector<std::pair<HMONITOR, RECT>> GetAllMonitorRects()
    {
        using result_t = std::vector<std::pair<HMONITOR, RECT>>;
        result_t result;

        auto enumMonitors = [](HMONITOR monitor, HDC hdc, LPRECT pRect, LPARAM param) -> BOOL {
            MONITORINFOEX mi;
            mi.cbSize = sizeof(mi);
            result_t& result = *reinterpret_cast<result_t*>(param);
            if (GetMonitorInfo(monitor, &mi))
            {
                result.push_back({ monitor, mi.*member });
            }

            return TRUE;
        };

        EnumDisplayMonitors(NULL, NULL, enumMonitors, reinterpret_cast<LPARAM>(&result));
        return result;
    }

    template<RECT MONITORINFO::*member>
    std::vector<std::pair<HMONITOR, MONITORINFOEX>> GetAllMonitorInfo()
    {
        using result_t = std::vector<std::pair<HMONITOR, MONITORINFOEX>>;
        result_t result;

        auto enumMonitors = [](HMONITOR monitor, HDC hdc, LPRECT pRect, LPARAM param) -> BOOL {
            MONITORINFOEX mi;
            mi.cbSize = sizeof(mi);
            result_t& result = *reinterpret_cast<result_t*>(param);
            if (GetMonitorInfo(monitor, &mi))
            {
                result.push_back({ monitor, mi });
            }

            return TRUE;
        };

        EnumDisplayMonitors(NULL, NULL, enumMonitors, reinterpret_cast<LPARAM>(&result));
        return result;
    }

    template<RECT MONITORINFO::*member>
    RECT GetAllMonitorsCombinedRect()
    {
        auto allMonitors = GetAllMonitorRects<member>();
        bool empty = true;
        RECT result{ 0, 0, 0, 0 };

        for (auto& [monitor, rect] : allMonitors)
        {
            if (empty)
            {
                empty = false;
                result = rect;
            }
            else
            {
                result.left = min(result.left, rect.left);
                result.top = min(result.top, rect.top);
                result.right = max(result.right, rect.right);
                result.bottom = max(result.bottom, rect.bottom);
            }
        }

        return result;
    }

    std::wstring GetDisplayDeviceId(const std::wstring& device, std::unordered_map<std::wstring, DWORD>& displayDeviceIdxMap);

    UINT GetDpiForMonitor(HMONITOR monitor) noexcept;
    void OrderMonitors(std::vector<std::pair<HMONITOR, RECT>>& monitorInfo);

    // Parameter rect must be in screen coordinates (e.g. obtained from GetWindowRect)
    void SizeWindowToRect(HWND window, RECT rect) noexcept;

    bool HasNoVisibleOwner(HWND window) noexcept;
    bool IsStandardWindow(HWND window);
    bool IsCandidateForLastKnownZone(HWND window, const std::vector<std::wstring>& excludedApps) noexcept;
    bool IsCandidateForZoning(HWND window, const std::vector<std::wstring>& excludedApps) noexcept;

    bool IsWindowMaximized(HWND window) noexcept;
    void SaveWindowSizeAndOrigin(HWND window) noexcept;
    void RestoreWindowSize(HWND window) noexcept;
    void RestoreWindowOrigin(HWND window) noexcept;

    bool IsValidGuid(const std::wstring& str);

    std::wstring GenerateUniqueId(HMONITOR monitor, const std::wstring& devideId, const std::wstring& virtualDesktopId);
    std::wstring GenerateUniqueIdAllMonitorsArea(const std::wstring& virtualDesktopId);

    std::wstring TrimDeviceId(const std::wstring& deviceId);
    std::optional<FancyZonesDataTypes::DeviceIdData> ParseDeviceId(const std::wstring& deviceId);
    bool IsValidDeviceId(const std::wstring& str);

    RECT PrepareRectForCycling(RECT windowRect, RECT zoneWindowRect, DWORD vkCode) noexcept;
    size_t ChooseNextZoneByPosition(DWORD vkCode, RECT windowRect, const std::vector<RECT>& zoneRects) noexcept;

    // If HWND is already dead, we assume it wasn't elevated
    bool IsProcessOfWindowElevated(HWND window);
}
