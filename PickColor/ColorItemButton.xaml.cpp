// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "ColorItemButton.xaml.h"
#if __has_include("ColorItemButton.g.cpp")
#include "ColorItemButton.g.cpp"
#endif

#include "winrt/Microsoft.UI.Composition.h"
#include "winrt/Microsoft.UI.Xaml.Hosting.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::PickColor::implementation
{
    ColorItemButton::ColorItemButton()
    {
        InitializeComponent();
    }

    void ColorItemButton::CreateDropShadow()
    {
        namespace mux = Microsoft::UI::Xaml;
        namespace muxh = mux::Hosting;

        if (auto element{ ShadowCaster() })
        {
            auto compositor{ muxh::ElementCompositionPreview::GetElementVisual(element).Compositor() };
            auto shadowVisual{ compositor.CreateSpriteVisual() };
            auto dropShadow{ compositor.CreateDropShadow() };

            shadowVisual.Shadow(dropShadow);

            Windows::Foundation::Numerics::float2 newSize{ 0.0f, 0.0f };
            if (auto fe{ element.try_as<mux::FrameworkElement>() })
            {
                newSize = Windows::Foundation::Numerics::float2(static_cast<float>(fe.ActualWidth()),
                                                                static_cast<float>(fe.ActualHeight()));
            }
            shadowVisual.Size(newSize);

            dropShadow.BlurRadius(8.0f);
            dropShadow.Opacity(0.35f);
            dropShadow.Offset(Windows::Foundation::Numerics::float3(0.0f, 3.0f, 0.0f));
            dropShadow.Color({ 255, 51, 51, 51 });

            muxh::ElementCompositionPreview::SetElementChildVisual(element, shadowVisual);
        }
    }

    void ColorItemButton::Color(Windows::UI::Color const& value)
    {
        if (m_color != value)
        {
            m_color = value;

            if (auto rect{ RectColor() })
            {
                rect.Color(value);

                CreateDropShadow();
            }
        }
    }
}
