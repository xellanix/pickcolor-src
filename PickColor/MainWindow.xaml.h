// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"
#include "ColorItem.g.h"
#include "ColorInfo.g.h"
#include "ColorViewModel.g.h"
#include "winrt/Microsoft.UI.Dispatching.h"
#include "winrt/Windows.UI.Core.h"
#include <wil/cppwinrt_helpers.h>
#include <dispatcherqueue.h>
#include "winrt/Microsoft.UI.Xaml.Controls.AnimatedVisuals.h"
#include "winrt/Microsoft.Windows.AppLifecycle.h"
#include "Utilities.h"
#include "ColorHelper.h"
#include <deque>

#define IS_ON_DEVELOPMENT_PROCESS 1

namespace winrt
{
    namespace MUC = Microsoft::UI::Composition;
    namespace MUX = Microsoft::UI::Xaml;
}

typedef COLORREF(WINAPI* GETPIXEL)(HDC, int, int);

namespace winrt::PickColor::implementation
{
    // IAsyncAction helper struct for queueing
    struct IAsyncQueue
    {
    private:
        using async_type = Windows::Foundation::IAsyncAction;

        async_type m_onGoing{ nullptr };
        bool m_isRunning = false;

        async_type StartAsyncQueue()
        {
            m_isRunning = true;

            while (m_onGoing)
            {
                async_type const current = m_onGoing;
                m_onGoing = nullptr;

                co_await current;
            }

            m_isRunning = false;
        }

    public:
        IAsyncQueue() = default;

        void Replace(async_type const value)
        {
            m_onGoing = value;

            if (!m_isRunning)
            {
                StartAsyncQueue();
            }
        }

        void Clear()
        {
            m_onGoing = nullptr;
        }
    };

    struct ColorInfo : ColorInfoT<ColorInfo>
    {
        ColorInfo() = default;
        ColorInfo(uint32_t const decimal) : m_colorDecimal(decimal)
        {}

        uint32_t ColorDecimal() const { return m_colorDecimal; }
        void ColorDecimal(uint32_t const value)
        {
            if (m_colorDecimal != value)
            {
                m_colorDecimal = value;
                m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"ColorDecimal" });
            }
        }

        winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
        {
            return m_propertyChanged.add(handler);
        }
        void PropertyChanged(winrt::event_token const& token) noexcept
        {
            m_propertyChanged.remove(token);
        }

    private:
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;

        uint32_t m_colorDecimal{ 0 };
    };

    struct ColorViewModel : ColorViewModelT<ColorViewModel>
    {
        ColorViewModel()
        {
            m_colorInfo = winrt::make<implementation::ColorInfo>(16777215);
        }

        PickColor::ColorInfo ColorInfo() const { return m_colorInfo; }

    private:
        PickColor::ColorInfo m_colorInfo{ nullptr };
    };

    struct ColorItem : ColorItemT<ColorItem>
    {
    private:
        uint32_t m_dec{ 0 };

    public:
        ColorItem() = default;

        ColorItem(uint32_t const& dec) : m_dec(dec)
        {}

        void Dec(uint32_t const& value)
        {
            m_dec = value;
        }
        uint32_t Dec() const
        {
            return m_dec;
        }

        winrt::Windows::UI::Color Color() const
        {
            return ColorHelper(m_dec).Color();
        }
    };

    enum struct GeneratedColorType
    {
        Shade,
        Tint
    };

    struct MainWindow : MainWindowT<MainWindow>
    {
    private:
        #pragma region Modern Title Bar & Background
        winrt::MUX::FrameworkElement::ActualThemeChanged_revoker m_themeChangedRevoker;
        winrt::MUX::FrameworkElement m_rootElement{ nullptr };
        bool isDarkMode = false;

        void SetModernAppTitleBar();
        double GetScaleAdjustment();
        void SetDragArea();
        bool GetCurrentTheme();
        void SetTitleBarTheme();
        #pragma endregion

        using ItemsT = Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable>;

        struct DropShadowOptions
        {
            float blurRadius = 8.0f;
            float opacity = 0.35f;
            Windows::Foundation::Numerics::float3 offset{ 0.0f, 2.5f, 0.0f };
            Windows::UI::Color color{ 255, 0, 0, 0 };
        };

        ItemsT m_colors;

        MUX::Controls::RadioButton m_userPickedColor{ nullptr };
        PickColor::ColorViewModel m_colorInfoModel{ nullptr };
        std::deque<Rgb> m_colorList;

        GeneratedColorType m_genType = GeneratedColorType::Shade;

        HINSTANCE _hGDI{ nullptr };
        bool isPointerMoves = false;
        Rgb m_lastColorRgb{ 0, 0, 0 };
        IAsyncQueue m_getRgbValueOps;
        bool isInNonClickArea = false;
        MUX::UIElement::PointerEntered_revoker pickButtonEntered;
        MUX::UIElement::PointerExited_revoker pickButtonExited;
        Windows::Foundation::IAsyncAction GetRGBValue(GETPIXEL pGetPixel, POINT _cursor);
        Windows::Foundation::IAsyncAction PointerMovesTask();

        void SetGeneratedColor(ColorHelper const& helper);
        void SetDetailedColorInfo(Windows::UI::Color const& color);
        std::pair<std::optional<bool>, std::optional<size_t>> InsertOrMove(Rgb const& value);
        void ManageColorItems(ColorHelper const& value);
        bool SaveColorHistory(fs::path const& path);

        Microsoft::Windows::AppLifecycle::AppInstance::Activated_revoker activationRevoker;
        Windows::Foundation::IAsyncAction AppInstanceActivated(Windows::Foundation::IInspectable const& sender, Microsoft::Windows::AppLifecycle::AppActivationArguments const& args);
        Windows::Foundation::IAsyncAction ProcessActivationInfo(Microsoft::Windows::AppLifecycle::AppActivationArguments args);
        Windows::Foundation::IAsyncAction GetColorNames();
        Windows::Foundation::IAsyncAction GetColorsHistory();

        void CreateDropShadow(MUX::UIElement const& uielement, DropShadowOptions const& options);

        template<typename T>
        fire_and_forget no_await(T t)
        {
            if constexpr (std::is_invocable_v<T>)
            {
                co_await t();
            }
            else
            {
                co_await t;
            }
        }

    public:
        MainWindow();
        
        ItemsT ColorsHistory() const { return m_colors; }
        void ColorsHistory(ItemsT const& value)
        {
            if (m_colors != value)
            {
                m_colors = value;
            }
        }

        PickColor::ColorViewModel ColorInfoViewModel() const { return m_colorInfoModel; }

        Windows::Foundation::IAsyncAction MainLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        Windows::Foundation::IAsyncAction PickScreenColor(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void FreezePickedColor(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void UserColorChecked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RecommendColorChecked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void LibraryColorChecked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void GenColorClicked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ChangeGenColorType(winrt::Microsoft::UI::Xaml::Controls::PipsPager const& sender, winrt::Microsoft::UI::Xaml::Controls::PipsPagerSelectedIndexChangedEventArgs const& args);

        void WindowSizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowSizeChangedEventArgs const& e);
        void WindowClosed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowEventArgs const& e);

        void SwitchByClick(winrt::Microsoft::UI::Xaml::Controls::SplitButton const& sender, winrt::Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs const& e);
        void SwitchByMenu(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::PickColor::factory_implementation
{
    struct ColorInfo : ColorInfoT<ColorInfo, implementation::ColorInfo>
    {};

    struct ColorViewModel : ColorViewModelT<ColorViewModel, implementation::ColorViewModel>
    {};

    struct ColorItem : ColorItemT<ColorItem, implementation::ColorItem>
    {};

    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
