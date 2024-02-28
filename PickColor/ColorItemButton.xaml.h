// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "ColorItemButton.g.h"
#include "winrt/Windows.UI.h"

namespace winrt::PickColor::implementation
{
    struct ColorItemButton : ColorItemButtonT<ColorItemButton>
    {
    private:
        Windows::UI::Color m_color{ Windows::UI::Colors::Transparent() };

        void CreateDropShadow();

    public:
        ColorItemButton();

        Windows::UI::Color Color() const { return m_color; }
        void Color(Windows::UI::Color const& value);
    };
}

namespace winrt::PickColor::factory_implementation
{
    struct ColorItemButton : ColorItemButtonT<ColorItemButton, implementation::ColorItemButton>
    {
    };
}
