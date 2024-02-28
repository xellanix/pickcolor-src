// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma region UserControls Headers & Sources

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#if __has_include("ColorItem.g.cpp")
#include "ColorItem.g.cpp"
#endif
#if __has_include("ColorInfo.g.cpp")
#include "ColorInfo.g.cpp"
#endif
#if __has_include("ColorViewModel.g.cpp")
#include "ColorViewModel.g.cpp"
#endif

#pragma endregion

#pragma region WinUI Headers

#include "microsoft.ui.xaml.window.h"
#include "winrt/Microsoft.UI.Interop.h"
#include "winrt/Microsoft.UI.Windowing.h"
#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.Hosting.h"
#include "winrt/Microsoft.UI.Xaml.Input.h"
#include "winrt/Microsoft.UI.Text.h"
#include "winrt/Windows.ApplicationModel.DataTransfer.h"

#pragma endregion

#pragma region Other Headers

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#include <random>
#include <Psapi.h>
#include <fstream>
#include <shellscalingapi.h>
#pragma comment(lib, "Shcore.lib")

#pragma endregion

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Windowing;

namespace muxc = Microsoft::UI::Xaml::Controls;
namespace mwal = Microsoft::Windows::AppLifecycle;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.
namespace winrt::PickColor::implementation
{
    MainWindow::MainWindow()
    {
        #if IS_ON_DEVELOPMENT_PROCESS

        ShellExecute(NULL, NULL, L"D:\\Beta Projects\\AppSystemUsage\\x64\\Debug\\AppSystemUsage.exe",
                     (std::to_wstring(GetCurrentProcessId())).c_str(),
                     NULL, SW_SHOWMINIMIZED);

        #endif // IS_ON_DEVELOPMENT_PROCESS

        InitializeComponent();

        m_colorInfoModel = make<ColorViewModel>();
        m_colors = single_threaded_observable_vector<Windows::Foundation::IInspectable>();

        SetModernAppTitleBar();
    }

    #pragma region Modern Title Section

    double GetScaleFactorFromHMonitor(HMONITOR const& hMonitor)
    {
        // Get DPI.
        UINT dpiX, dpiY;
        int result = GetDpiForMonitor(hMonitor, MDT_DEFAULT, &dpiX, &dpiY);
        if (result != S_OK)
        {
            return 1.0;
        }

        auto scaleFactorPercent = dpiX / 96.0;
        return scaleFactorPercent;
    }

    void MainWindow::SetModernAppTitleBar()
    {
        auto windowNative{ this->try_as<::IWindowNative>() };
        winrt::check_bool(windowNative);
        windowNative->get_WindowHandle(&Xellanix::Desktop::WindowHandle);

        Microsoft::UI::WindowId windowId =
            Microsoft::UI::GetWindowIdFromWindow(Xellanix::Desktop::WindowHandle);

        Microsoft::UI::Windowing::AppWindow appWindow =
            Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);

        if (auto presenter = appWindow.Presenter().try_as<OverlappedPresenter>())
        {
            HMONITOR hMonitor;
            {
                POINT _cursor;
                if (GetCursorPos(&_cursor) == FALSE) _cursor = { 0, 0 };
                hMonitor = MonitorFromPoint(_cursor, MONITOR_DEFAULTTONEAREST);
            }

            // The window size if the DPI Scale Factor is 1.25 (125%)
            auto windowSize = Windows::Graphics::SizeInt32{ 565, 680 };
            {
                auto scaleAdj = GetScaleFactorFromHMonitor(hMonitor);
                
                auto newWidth = static_cast<int32_t>(windowSize.Width / 1.25 * scaleAdj);
                auto newHeight = static_cast<int32_t>(windowSize.Height / 1.25 * scaleAdj);

                windowSize = Windows::Graphics::SizeInt32{ newWidth, newHeight };
            }
            appWindow.Resize(windowSize);

            MONITORINFO info{};
            info.cbSize = sizeof(decltype(info));

            if (GetMonitorInfo(hMonitor, &info) != 0)
            {
                const auto width = windowSize.Width + 10;
                const auto height = windowSize.Height + 10;
                auto px = (info.rcWork.right - width);
                auto py = (info.rcWork.bottom - height);

                appWindow.Move(Windows::Graphics::PointInt32{ px, py });
            }

            SetWindowPos(Xellanix::Desktop::WindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            presenter.IsResizable(false);
            presenter.IsMaximizable(false);
            presenter.IsMinimizable(false);
        }

        if (AppWindowTitleBar::IsCustomizationSupported())
        {
            auto titleBar{ appWindow.TitleBar() };

            titleBar.ExtendsContentIntoTitleBar(true);
            titleBar.ButtonBackgroundColor(Microsoft::UI::Colors::Transparent());
            titleBar.ButtonInactiveBackgroundColor(Microsoft::UI::Colors::Transparent());
        }
        else
        {
            // Title bar customization using these APIs is currently
            // supported only on Windows 11. In other cases, hide
            // the custom title bar element.            
            AppTitleBar().Visibility(Visibility::Collapsed);

            // Show alternative UI for any functionality in
            // the title bar, such as search.
        }

        if (appWindow)
        {
            // You now have an AppWindow object, and you can call its methods to manipulate the window.
            // As an example, let's change the title text of the window.
            appWindow.Title(L"Xellanix PickColor");
            SetTitleBarTheme();
        }
    }

    double MainWindow::GetScaleAdjustment()
    {
        Microsoft::UI::WindowId windowId =
            Microsoft::UI::GetWindowIdFromWindow(Xellanix::Desktop::WindowHandle);
        
        DisplayArea displayArea = DisplayArea::GetFromWindowId(windowId, DisplayAreaFallback::Nearest);
        auto hMonitor = Microsoft::UI::GetMonitorFromDisplayId(displayArea.DisplayId());

        return GetScaleFactorFromHMonitor(hMonitor);
    }

    void MainWindow::SetDragArea()
    {
        auto scaleAdj = GetScaleAdjustment();

        Microsoft::UI::WindowId windowId =
            Microsoft::UI::GetWindowIdFromWindow(Xellanix::Desktop::WindowHandle);

        Microsoft::UI::Windowing::AppWindow appWindow =
            Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);

        if (AppWindowTitleBar::IsCustomizationSupported())
        {
            auto titleBar{ appWindow.TitleBar() };

            const auto buttonWidth = GetSystemMetrics(SM_CXSIZE);
            RightPaddingColumn().Width(MUX::GridLengthHelper::FromPixels(buttonWidth));

            const auto dragWidth = static_cast<int32_t>(LeftDragColumn().ActualWidth() * scaleAdj);
            const auto dragHeight = static_cast<int32_t>(32 * scaleAdj);
            titleBar.SetDragRectangles({ Windows::Graphics::RectInt32{ 0, 0, dragWidth, dragHeight } });
        }
        else
        {
            // Title bar customization using these APIs is currently
            // supported only on Windows 11. In other cases, hide
            // the custom title bar element.            
            AppTitleBar().Visibility(Visibility::Collapsed);

            // Show alternative UI for any functionality in
            // the title bar, such as search.
        }

        HMONITOR hMonitor;
        {
            POINT _cursor;
            if (GetCursorPos(&_cursor) == FALSE) _cursor = { 0, 0 };
            hMonitor = MonitorFromPoint(_cursor, MONITOR_DEFAULTTONEAREST);
        }

        MONITORINFO info{};
        info.cbSize = sizeof(decltype(info));

        if (GetMonitorInfo(hMonitor, &info) != 0)
        {
            const auto windowSize = appWindow.Size();

            const auto width = windowSize.Width + 10;
            const auto height = windowSize.Height + 10;
            auto px = (info.rcWork.right - width);
            auto py = (info.rcWork.bottom - height);

            appWindow.Move(Windows::Graphics::PointInt32{ px, py });
        }
    }
    
    bool MainWindow::GetCurrentTheme()
    {
        bool isCurrentDarkMode = false;
        if (auto theme{ m_rootElement.RequestedTheme() }; theme == ElementTheme::Dark)
        {
            isCurrentDarkMode = true;
        }
        else if (theme == ElementTheme::Light)
        {
            isCurrentDarkMode = false;
        }
        else
        {
            if (auto currentApp{ Application::Current() })
            {
                isCurrentDarkMode = currentApp.RequestedTheme() == ApplicationTheme::Dark;
            }
        }

        return isCurrentDarkMode;
    }

    void MainWindow::SetTitleBarTheme()
    {
        // Application theme.
        m_rootElement = this->Content().try_as<winrt::MUX::FrameworkElement>();
        if (nullptr != m_rootElement)
        {
            m_themeChangedRevoker = m_rootElement.ActualThemeChanged(winrt::auto_revoke, [&](auto&&, auto&&)
            {
                bool isCurrentDarkMode = GetCurrentTheme();
                if (isDarkMode == isCurrentDarkMode) return;
                isDarkMode = isCurrentDarkMode;

                BOOL value = isDarkMode;

                ::DwmSetWindowAttribute(Xellanix::Desktop::WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

                if (AppWindowTitleBar::IsCustomizationSupported())
                {
                    auto foregroundColor{ Microsoft::UI::ColorHelper::FromArgb(255, 48, 48, 48) };
                    auto hoverBackground{ Microsoft::UI::ColorHelper::FromArgb(255, 230, 230, 230) };
                    auto preseedBackground{ Microsoft::UI::ColorHelper::FromArgb(255, 242, 242, 242) };
                    if (isDarkMode)
                    {
                        foregroundColor = Microsoft::UI::ColorHelper::FromArgb(255, 245, 245, 247);
                        hoverBackground = Microsoft::UI::ColorHelper::FromArgb(255, 51, 51, 51);
                        preseedBackground = Microsoft::UI::ColorHelper::FromArgb(255, 38, 38, 38);
                    }

                    Microsoft::UI::WindowId windowId =
                        Microsoft::UI::GetWindowIdFromWindow(Xellanix::Desktop::WindowHandle);

                    Microsoft::UI::Windowing::AppWindow appWindow =
                        Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);

                    auto titleBar{ appWindow.TitleBar() };

                    titleBar.ButtonForegroundColor(foregroundColor);
                    titleBar.ButtonHoverForegroundColor(foregroundColor);
                    titleBar.ButtonHoverBackgroundColor(hoverBackground);
                    titleBar.ButtonPressedForegroundColor(foregroundColor);
                    titleBar.ButtonPressedBackgroundColor(preseedBackground);
                    titleBar.ButtonInactiveForegroundColor(foregroundColor);
                }
            });
        }
    }

    #pragma endregion

    #pragma region Load Data (Files) Events

    std::wstring GetDataFromFile(const fs::path& path)
    {
        std::wifstream stream;
        stream.open(path, std::ios::binary);

        std::wstring data;
        {
            std::wostringstream wos;
            wos << stream.rdbuf();
            data = wos.str();
        }

        stream.close();

        return data;
    }

    std::optional<std::wstring_view> GetLine(std::wstring_view const data, size_t& index)
    {
        size_t startIndex = index;
        size_t endIndex = data.find(L"\n", startIndex);

        if (endIndex != std::wstring_view::npos)
        {
            index = endIndex + 1;

            if (endIndex > 0)
            {
                auto before = data[endIndex - 1];
                if (before == L'\r') --endIndex;
            }
            while (endIndex <= startIndex)
            {
                endIndex = data.find(L"\n", ++startIndex);

                if (endIndex == std::wstring_view::npos) return std::nullopt;
                else
                {
                    if (endIndex > startIndex)
                    {
                        auto before = data[endIndex - 1];
                        if (before == L'\r') --endIndex;
                    }
                }
            }

            auto length = endIndex - startIndex;
            return data.substr(startIndex, length);
        }
        else
        {
            index = endIndex;
            return std::nullopt;
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::GetColorNames()
    {
        co_await resume_background();

        auto data = GetDataFromFile((fs::path(Xellanix::Utilities::AppDir) / L"ColorList.csv"));

        size_t lineIndex = 0;
        while (auto lineOpt{ GetLine(data, lineIndex) })
        {
            auto&& line = std::move(lineOpt.value());
            if (line.empty()) continue;

            size_t ci = line.find(L",", 0);
            double h, s, v;
            {
                std::array<double, 3> values{ 0.0, 0.0, 0.0 };
                hstring colorName{ line.substr(0, ci) };

                for (uint8_t order = 0; ci != std::wstring_view::npos && order < 3; order++)
                {
                    auto nci = line.find(L",", ci + 1);

                    hstring value{ line.substr(ci + 1, nci - ci - 1) };
                    values[order] = _wtof(value.c_str());

                    ci = nci;
                }

                h = values[0];
                s = values[1];
                v = values[2];

                ColorNames[h][s][v] = colorName;
            }
        }
    }

    #pragma endregion

    #pragma region Root Control Loaded

    Windows::Foundation::IAsyncAction MainWindow::ProcessActivationInfo(mwal::AppActivationArguments args = mwal::AppInstance::GetCurrent().GetActivatedEventArgs())
    {
        using namespace mwal;

        auto weak{ get_weak() };
        co_await resume_background();

        if (args.Kind() == ExtendedActivationKind::Launch)
        {
            if (auto launchArgs{ args.Data().try_as<Windows::ApplicationModel::Activation::ILaunchActivatedEventArgs>() })
            {
                int nArgs;
                LPWSTR* parsed;
                {
                    std::wstring argString{ launchArgs.Arguments() };
                    parsed = CommandLineToArgvW(argString.c_str(), &nArgs);
                }

                if (nArgs >= 2)
                {
                    std::wstring key{ parsed[1] };
                    LocalFree(parsed);

                    if (key == L"e2b7a873-937e-5b53-8242-c7b07ba6134f")
                    {
                        if (auto strong{ weak.get() })
                        {
                            if (auto btn{ strong->PickColorButton() })
                            {
                                co_await wil::resume_foreground(strong->DispatcherQueue());
                                btn.IsChecked(true);
                            }
                        }
                    }
                }
                else
                {
                    LocalFree(parsed);
                }
            }
        }
    }
    
    Windows::Foundation::IAsyncAction MainWindow::AppInstanceActivated(Windows::Foundation::IInspectable const& /*sender*/, Microsoft::Windows::AppLifecycle::AppActivationArguments const& args)
    {
        co_await ProcessActivationInfo(args);
    }

    Windows::Foundation::IAsyncAction MainWindow::GetColorsHistory()
    {
        auto strong{ get_strong() };
        co_await resume_background();

        std::deque<ColorHelper> colorTemp;

        if (auto const path = Xellanix::Utilities::LocalAppData / L"history.xpch";
            Xellanix::Utilities::CheckExist(path))
        {
            auto data = GetDataFromFile(path);
        }

        if (colorTemp.empty())
        {
            for (uint32_t i = 0; i < 6; i++)
            {
                std::random_device rd;
                std::mt19937_64 gen(rd());
                std::uniform_int_distribution<uint32_t> dis(0, 16'777'215);
                auto res = dis(gen);

                colorTemp.emplace_front(res);
            }
        }

        co_await wil::resume_foreground(DispatcherQueue());
        for (auto const& i : colorTemp)
        {
            ManageColorItems(i);
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::MainLoaded(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        auto strong{ get_strong() };

        SetDragArea();
        isDarkMode = GetCurrentTheme();

        CreateDropShadow(GenColorViewShadowCaster(), DropShadowOptions{ 12.0f, 0.35f, { 0.0f, 0.25f, 0.0f }, { 255, 77, 77, 77 } });
        
        muxc::InfoBar infobar{ InfobarControl() };
        infobar.Visibility(Visibility::Visible);
        infobar.IsOpen(true);

        #pragma region Get Colors Name

        infobar.Message(L"Colors name");
        co_await GetColorNames();

        #pragma endregion

        #pragma region Get Colors History

        infobar.Message(L"Colors history");
        co_await GetColorsHistory();

        if (auto container{ CurrentColorList() })
        {
            auto list{ container.Children() };

            list.GetAt(1).try_as<PickColor::ColorItemButton>().IsChecked(true);
        }

        #pragma endregion
        
        infobar.IsOpen(false);
        infobar.Visibility(Visibility::Collapsed);

        no_await([this]() -> Windows::Foundation::IAsyncAction
        {
            auto strong_lambda{ get_strong() };

            co_await ProcessActivationInfo();

            using namespace Microsoft::Windows::AppLifecycle;
            activationRevoker = AppInstance::GetCurrent().Activated(auto_revoke, { get_weak(), &MainWindow::AppInstanceActivated });
        });

        #pragma region Temporary

        SetWindowPos(Xellanix::Desktop::WindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(Xellanix::Desktop::WindowHandle);

        #pragma endregion
    }

    #pragma endregion

    #pragma region UI/UX Generator

    void MainWindow::CreateDropShadow(MUX::UIElement const& uielement, DropShadowOptions const& options = {})
    {
        namespace muxh = MUX::Hosting;

        if (auto element{ uielement })
        {
            auto compositor{ muxh::ElementCompositionPreview::GetElementVisual(element).Compositor() };
            auto shadowVisual{ compositor.CreateSpriteVisual() };
            auto dropShadow{ compositor.CreateDropShadow() };

            shadowVisual.Shadow(dropShadow);

            Windows::Foundation::Numerics::float2 newSize{ 0.0f, 0.0f };
            if (auto fe{ element.try_as<MUX::FrameworkElement>() })
            {
                newSize = Windows::Foundation::Numerics::float2(static_cast<float>(fe.ActualWidth()),
                                                                static_cast<float>(fe.ActualHeight()));
            }
            shadowVisual.Size(newSize);

            dropShadow.BlurRadius(options.blurRadius);
            dropShadow.Opacity(options.opacity);
            dropShadow.Offset(options.offset);
            dropShadow.Color(options.color);

            muxh::ElementCompositionPreview::SetElementChildVisual(element, shadowVisual);
        }
    }

    #pragma endregion

    #pragma region Color Info & Generations

    void MainWindow::SetGeneratedColor(ColorHelper const& helper)
    {
        if (muxc::VariableSizedWrapGrid view{ GenColorView() })
        {
            if (auto main{ view.Children().GetAt(0).try_as<muxc::Button>() })
            {
                main.Background(MUX::Media::SolidColorBrush{ helper.Color() });
                Controls::ToolTipService::SetToolTip(main, box_value(L"#" + helper.Hex()));
            }

            auto [h, s, v] = helper.Hsv();

            double bH = 0.0, bS = 0.0, bV = 0.0;
            switch (m_genType)
            {
                case winrt::PickColor::implementation::GeneratedColorType::Shade:
                    bV = v / 10.0;
                    break;
                case winrt::PickColor::implementation::GeneratedColorType::Tint:
                    bS = s / 10.0;
                    bV = (100.0 - v) / -10.0;
                    break;
                default:
                    bV = v / 10.0;
                    break;
            }

            for (uint32_t i = 1; i <= 11; i++)
            {
                h -= bH;
                s -= bS;
                v -= bV;

                if (auto gen{ view.Children().GetAt(i).try_as<muxc::Button>() })
                {
                    auto nhelper = ColorHelper(Hsv{ h, s, v });
                    gen.Background(MUX::Media::SolidColorBrush{ nhelper.Color() });
                    Controls::ToolTipService::SetToolTip(gen, box_value(L"#" + nhelper.Hex()));
                }
            }
        }
    }

    void MainWindow::SetDetailedColorInfo(Windows::UI::Color const& color)
    {
        auto helper{ ColorHelper(color) };

        m_lastColorRgb = helper;
        ColorInfoViewModel().ColorInfo().ColorDecimal(helper);

        SetGeneratedColor(helper);
    }

    std::pair<std::optional<bool>, std::optional<size_t>> MainWindow::InsertOrMove(Rgb const& value)
    {
        if (auto res = std::find(m_colorList.begin(), m_colorList.end(), value); res != m_colorList.end())
        {
            auto index = std::distance(m_colorList.begin(), res);

            if (index != 0)
            {
                std::rotate(m_colorList.begin(), res, res + 1);
                return { true, index };
            }

            return { std::nullopt, std::nullopt };
        }
        
        m_colorList.emplace_front(value);

        if (m_colorList.size() > 6)
        {
            return { false, 0 };
        }

        return { false, std::nullopt };
    }

    void MainWindow::ManageColorItems(ColorHelper const& value)
    {
        const auto&& [isMovedOpt, indexOpt] = InsertOrMove(value);

        if (!isMovedOpt.has_value()) return;

        const auto isMoved = isMovedOpt.value();
        if (isMoved)
        {
            auto index = static_cast<uint32_t>(indexOpt.value_or(0));
            if (index > 5)
            {
                index -= 6;

                m_colors.RemoveAt(index);
                m_colors.InsertAt(0, make<ColorItem>(ColorHelper(m_colorList[6]).Decimal()));

                if (auto container{ CurrentColorList() })
                {
                    auto list{ container.Children() };

                    PickColor::ColorItemButton btn{};
                    btn.Color(value);
                    btn.Checked({ get_weak(), &MainWindow::RecommendColorChecked });

                    list.RemoveAtEnd();
                    list.InsertAt(1, btn);
                }
            }
            else
            {
                if (auto container{ CurrentColorList() })
                {
                    auto list{ container.Children() };

                    list.Move(index + 1, 1);
                }
            }
        }
        else
        {
            bool _value = indexOpt.has_value();

            if (auto container{ CurrentColorList() })
            {
                auto list{ container.Children() };

                PickColor::ColorItemButton btn{};
                btn.Color(value);
                btn.Checked({ get_weak(), &MainWindow::RecommendColorChecked });

                if (_value) list.RemoveAtEnd();
                list.InsertAt(1, btn);
            }

            if (_value)
            {
                m_colors.InsertAt(0, make<ColorItem>(ColorHelper(m_colorList[6]).Decimal()));
            }
        }
    }

    bool MainWindow::SaveColorHistory(fs::path const& path = Xellanix::Utilities::LocalAppData / L"history.xpch")
    {
        try
        {
            if (auto const pp = path.parent_path(); !Xellanix::Utilities::CheckExist(pp))
                std::filesystem::create_directories(pp);

            std::wstring output;

            for (auto const& i : m_colorList)
            {
                auto const&& [r, g, b] = i;
                output += r;
                output += g;
                output += b;
            }

            std::wofstream wof;
            wof.open(path, std::ios::binary);
            wof << output;
            wof.close();

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    #pragma endregion
    
    #pragma region Color Picker Functions

    Windows::Foundation::IAsyncAction MainWindow::GetRGBValue(GETPIXEL pGetPixel, POINT _cursor)
    {
        auto cancellation = co_await get_cancellation_token();
        cancellation.enable_propagation();
        auto strong{ get_strong() };

        HDC _hdc = GetDC(NULL);
        if (_hdc)
        {
            COLORREF _color = (pGetPixel)(_hdc, _cursor.x, _cursor.y);
            byte _red = GetRValue(_color);
            byte _green = GetGValue(_color);
            byte _blue = GetBValue(_color);

            Rgb _rgb{ _red, _green, _blue };
            if (_rgb == strong->m_lastColorRgb || cancellation())
            {
                ReleaseDC(NULL, _hdc);
                co_return;
            }

            co_await wil::resume_foreground(strong->DispatcherQueue());
            strong->SetDetailedColorInfo(ColorHelper(_rgb));

            if (ReleaseDC(NULL, _hdc) == 0) __debugbreak();
        }

        co_return;
    }

    Windows::Foundation::IAsyncAction MainWindow::PointerMovesTask()
    {
        auto strong{ get_strong() };
        co_await resume_background();

        strong->_hGDI = LoadLibrary(L"gdi32.dll");
        if (_hGDI)
        {
            GETPIXEL pGetPixel = (GETPIXEL)GetProcAddress(strong->_hGDI, "GetPixel");

            auto IsKeyDown = [](int key) -> bool
            {
                return (GetAsyncKeyState(key) & 0x8000) != 0;
            };

            POINT lastCursor{ 0, 0 };
            while (strong->isPointerMoves)
            {
                POINT _cursor;
                GetCursorPos(&_cursor);

                if (IsKeyDown(VK_ESCAPE) || 
                    (IsKeyDown(GetSystemMetrics(SM_SWAPBUTTON) == 0 ? VK_LBUTTON : VK_RBUTTON) && !strong->isInNonClickArea))
                {
                    if (muxc::Primitives::ToggleButton btn{ strong->PickColorButton() })
                    {
                        co_await wil::resume_foreground(strong->DispatcherQueue());
                        btn.IsChecked(false);
                    }
                }

                if (_cursor.x == lastCursor.x && _cursor.y == lastCursor.y) continue;
                lastCursor = _cursor;

                strong->m_getRgbValueOps.Replace(GetRGBValue(pGetPixel, _cursor));
            }
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::PickScreenColor(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        SetWindowPos(Xellanix::Desktop::WindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(Xellanix::Desktop::WindowHandle);

        if (muxc::Primitives::ToggleButton btn{ PickColorButton() })
        {
            pickButtonEntered = btn.PointerEntered(auto_revoke, [strong{ get_strong() }](auto&&, auto&&)
            {
                strong->isInNonClickArea = true;
            });

            pickButtonExited = btn.PointerExited(auto_revoke, [strong{ get_strong() }](auto&&, auto&&)
            {
                strong->isInNonClickArea = false;
            });

            isPointerMoves = true;
            co_await PointerMovesTask();
        }
    }

    void MainWindow::FreezePickedColor(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        isPointerMoves = false;
        if (_hGDI)
        {
            FreeLibrary(_hGDI);
        }
        pickButtonEntered.revoke();
        pickButtonExited.revoke();

        ManageColorItems(m_lastColorRgb);
        if (auto container{ CurrentColorList() })
        {
            auto list{ container.Children() };

            list.GetAt(1).try_as<PickColor::ColorItemButton>().IsChecked(true);
        }

        /*for (size_t i = 0; i < 30; i++)
        {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint32_t> dis(0, 16'777'215);

            m_colors.Append(make<ColorItem>(dis(gen)));
        }*/
    }

    #pragma endregion

    #pragma region GenColor Events

    void MainWindow::GenColorClicked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        if (auto btn{ sender.try_as<muxc::Button>() })
        {
            if (auto brush{ btn.Background().try_as<MUX::Media::SolidColorBrush>() })
            {
                auto helper{ ColorHelper(brush.Color()) };

                ColorInfoViewModel().ColorInfo().ColorDecimal(helper);
            }
        }
    }

    void MainWindow::ChangeGenColorType(winrt::Microsoft::UI::Xaml::Controls::PipsPager const& sender, winrt::Microsoft::UI::Xaml::Controls::PipsPagerSelectedIndexChangedEventArgs const& /*args*/)
    {
        if (sender)
        {
            auto index = sender.SelectedPageIndex();
            m_genType = static_cast<GeneratedColorType>(index);

            if (auto title{ GenColorTypeTitle() })
            {
                if (index == 0)
                {
                    title.Text(L"Color Shade");
                }
                else if (index == 1)
                {
                    title.Text(L"Color Tint");
                }
            }

            SetGeneratedColor(ColorInfoViewModel().ColorInfo().ColorDecimal());
        }
    }

    #pragma endregion

    #pragma region Color History Selection

    void MainWindow::UserColorChecked(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        if (m_userPickedColor)
        {
            m_userPickedColor.IsChecked(true);
        }

        if (auto control{ UserColorSelection() })
        {
            SetDetailedColorInfo(control.Color());
        }
    }

    void MainWindow::RecommendColorChecked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        if (auto selection = UserColorSelection())
        {
            if (auto checked = selection.IsChecked(); checked && checked.GetBoolean())
                selection.IsChecked(false);
        }

        if (auto control{ sender.try_as<PickColor::ColorItemButton>() })
        {
            SetDetailedColorInfo(control.Color());
        }
    }

    void MainWindow::LibraryColorChecked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        m_userPickedColor = sender.try_as<muxc::RadioButton>();

        if (auto selection = UserColorSelection())
        {
            auto color = m_userPickedColor.Content().try_as<MUX::Shapes::Rectangle>().Fill();
            if (auto rect{ m_userPickedColor.Content().try_as<MUX::Shapes::Rectangle>() })
            {
                if (auto brush{ rect.Fill().try_as<MUX::Media::SolidColorBrush>() })
                {
                    selection.Color(brush.Color());
                }
            }

            if (selection.Visibility() == MUX::Visibility::Collapsed)
            {
                selection.Visibility(MUX::Visibility::Visible);
            }

            if (auto checked = selection.IsChecked(); !checked || !checked.GetBoolean())
            {
                selection.IsChecked(true);
            }
        }
    }

    #pragma endregion

    #pragma region Window Events

    void MainWindow::WindowSizeChanged(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::WindowSizeChangedEventArgs const& /*e*/)
    {
        SetDragArea();
    }

    void MainWindow::WindowClosed(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::WindowEventArgs const& /*e*/)
    {
        //MessageBox(NULL, L"Closed", 0, 0);
        using namespace Microsoft::Windows::AppLifecycle;
        activationRevoker.revoke();
        AppInstance::GetCurrent().UnregisterKey();
    }

    #pragma endregion

    #pragma region Window Theme Switcher

    void MainWindow::SwitchByClick(winrt::Microsoft::UI::Xaml::Controls::SplitButton const& /*sender*/, winrt::Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs const& /*e*/)
    {
        if (isDarkMode)
        {
            m_rootElement.RequestedTheme(ElementTheme::Light);

            if (auto icon{ ThemeIcon() })
            {
                icon.Style(unbox_value<MUX::Style>(m_rootElement.Resources().Lookup(box_value(L"LightThemeIconStyle"))));
            }
        }
        else
        {
            m_rootElement.RequestedTheme(ElementTheme::Dark);

            if (auto icon{ ThemeIcon() })
            {
                icon.Style(unbox_value<MUX::Style>(m_rootElement.Resources().Lookup(box_value(L"DarkThemeIconStyle"))));
            }
        }
    }

    void MainWindow::SwitchByMenu(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        if (auto item{ sender.try_as<muxc::MenuFlyoutItem>() })
        {
            auto mode = item.Text();

            if (auto theme = m_rootElement.RequestedTheme(); mode == L"System Default")
            {
                if (theme == ElementTheme::Default) return;

                m_rootElement.RequestedTheme(ElementTheme::Default);

                if (auto icon{ ThemeIcon() })
                {
                    icon.Style(unbox_value<MUX::Style>(m_rootElement.Resources().Lookup(box_value(L"DefaultThemeIconStyle"))));
                }
            }
            else if (mode == L"Dark")
            {
                if (theme == ElementTheme::Dark) return;

                m_rootElement.RequestedTheme(ElementTheme::Dark);

                if (auto icon{ ThemeIcon() })
                {
                    icon.Style(unbox_value<MUX::Style>(m_rootElement.Resources().Lookup(box_value(L"DarkThemeIconStyle"))));
                }
            }
            else if (mode == L"Light")
            {
                if (theme == ElementTheme::Light) return;

                m_rootElement.RequestedTheme(ElementTheme::Light);

                if (auto icon{ ThemeIcon() })
                {
                    icon.Style(unbox_value<MUX::Style>(m_rootElement.Resources().Lookup(box_value(L"LightThemeIconStyle"))));
                }
            }
        }
    }

    #pragma endregion

    #pragma region Others

    #pragma endregion
}
