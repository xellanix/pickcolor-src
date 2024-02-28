// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "ColorInfoBar.g.h"
#include "ColorHelper.h"

namespace winrt::PickColor::implementation
{
    struct ColorInfoBar : ColorInfoBarT<ColorInfoBar>
    {
    private:
        hstring m_typeTitle{ L"" };
        uint32_t m_colorDecimal{ 16'777'215 };

    public:
        ColorInfoBar();

        hstring TypeTitle() const { return m_typeTitle; }
        void TypeTitle(hstring const& value);

        uint32_t ColorDecimal() const { return m_colorDecimal; }
        void ColorDecimal(uint32_t const value);

        void CopyValue(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CopyWithTitle(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CopyTemporarily(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::PickColor::factory_implementation
{
    struct ColorInfoBar : ColorInfoBarT<ColorInfoBar, implementation::ColorInfoBar>
    {
    };
}
