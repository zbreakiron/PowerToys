#pragma once

#include <map>
#include <vector>
#include <wil\resource.h>
#include <winrt/base.h>
#include <d2d1.h>
#include <dwrite.h>

#include "util.h"
#include "Zone.h"
#include "ZoneSet.h"
#include "FancyZones.h"

class ZoneWindowDrawing
{
    struct DrawableRect
    {
        D2D1_RECT_F rect;
        D2D1_COLOR_F borderColor;
        D2D1_COLOR_F fillColor;
        size_t id;
    };

    struct AnimationInfo
    {
        std::chrono::steady_clock::time_point tStart;
        bool autoHide;
    };

    enum struct RenderResult
    {
        Ok,
        AnimationEnded,
        Failed,
    };

    HWND m_window = nullptr;
    RECT m_clientRect{};
    ID2D1HwndRenderTarget* m_renderTarget = nullptr;
    std::optional<AnimationInfo> m_animation;

    std::mutex m_mutex;
    std::vector<DrawableRect> m_sceneRects;

    float GetAnimationAlpha();
    static ID2D1Factory* GetD2DFactory();
    static IDWriteFactory* GetWriteFactory();
    static D2D1_COLOR_F ConvertColor(COLORREF color);
    static D2D1_RECT_F ConvertRect(RECT rect);
    RenderResult Render();
    void RenderLoop();

    std::atomic<bool> m_shouldRender = false;
    std::atomic<bool> m_abortThread = false;
    std::condition_variable m_cv;
    std::thread m_renderThread;

public:

    ~ZoneWindowDrawing();
    ZoneWindowDrawing(HWND window);
    void Hide();
    void Show();
    void Flash();
    void DrawActiveZoneSet(const IZoneSet::ZonesMap& zones,
                           const std::vector<size_t>& highlightZones,
                           winrt::com_ptr<IZoneWindowHost> host);
};
