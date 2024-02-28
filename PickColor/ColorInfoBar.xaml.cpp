// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "ColorInfoBar.xaml.h"
#if __has_include("ColorInfoBar.g.cpp")
#include "ColorInfoBar.g.cpp"
#endif

#include "winrt/Windows.ApplicationModel.DataTransfer.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::PickColor::implementation
{
    ColorInfoBar::ColorInfoBar()
    {
        InitializeComponent();
    }

    void ColorInfoBar::TypeTitle(hstring const& value)
    {
        if (m_typeTitle != value)
        {
            m_typeTitle = value;

            if (auto text{ TypeTitleText() })
            {
                text.Text(value);
                Controls::ToolTipService::SetToolTip(text, box_value(value));
            }
        }
    }

    void ColorInfoBar::ColorDecimal(uint32_t const value)
    {
        if (m_colorDecimal != value)
        {
            m_colorDecimal = value;

            if (auto text{ TypeValueText() })
            {
                const auto decimal = value;

                auto const helper{ ColorHelper(decimal) };
                auto const hsv = helper.Hsv();

                if (m_typeTitle == L"HEX")
                {
                    hstring result{ helper.Hex() };
                    text.Text(result);
                }
                else if (m_typeTitle == L"RGB")
                {
                    auto const&& [r, g, b] = helper.Rgb();

                    hstring result = to_hstring(r) + L", " + to_hstring(g) + L", " + to_hstring(b);
                    text.Text(result);
                }
                else if (m_typeTitle == L"HSV")
                {
                    auto const& [h, s, v] = hsv;

                    auto doubleToString = [](double d)
                    {
                        return to_hstring(round(d * 100.0) / 100.0);
                    };

                    hstring result = doubleToString(h) + L"°, " + doubleToString(s) + L"%, " + doubleToString(v) + L"%";
                    text.Text(result);
                }
                else if (m_typeTitle == L"Name")
                {
                    hstring result{ helper.ColorName(hsv) };
                    text.Text(result);
                }
                else
                {
                    text.Text(to_hstring(decimal));
                }

                Controls::ToolTipService::SetToolTip(text, box_value(text.Text()));
            }
        }
    }

    void ColorInfoBar::CopyValue(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        namespace wamdt = Windows::ApplicationModel::DataTransfer;

        if (auto text{ TypeValueText() })
        {
            wamdt::DataPackage package{};
            package.SetText(text.Text());
            wamdt::Clipboard::SetContent(package);
        }
    }

    void ColorInfoBar::CopyWithTitle(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        namespace wamdt = Windows::ApplicationModel::DataTransfer;

        if (auto text{ TypeValueText() })
        {
            std::wstring rtf{ LR"myrtf({\rtf1\fbidis\ansi\ansicpg1252\deff0\nouicompat\deflang1033{\fonttbl{\f0\fnil\fcharset0 Segoe UI;}{\f1\fnil Segoe UI;}}
{\colortbl ;\red0\green0\blue0;}
{\*\generator Riched20 3.1.0003}\viewkind4\uc1 
\pard\tx720\cf1\b\f0\fs21 )myrtf" };
            rtf += m_typeTitle;
            rtf += LR"myrtf(\b0 : )myrtf";
            rtf += text.Text();
            rtf += LR"myrtf(\f1\par
}
)myrtf";

            wamdt::DataPackage package{};
            package.SetRtf(rtf);
            wamdt::Clipboard::SetContent(package);
        }
    }

    void ColorInfoBar::CopyTemporarily(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        namespace wamdt = Windows::ApplicationModel::DataTransfer;

        if (auto text{ TypeValueText() })
        {
            wamdt::DataPackage package{};
            package.SetText(text.Text());
            wamdt::Clipboard::SetContent(package);
        }
    }
}
