// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"

#include "App.xaml.h"
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace PickColor;
using namespace PickColor::implementation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](Windows::Foundation::IInspectable const&, UnhandledExceptionEventArgs const& e)
    {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

/// <summary>
/// Invoked when the application is launched.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
Windows::Foundation::IAsyncAction App::OnLaunched(LaunchActivatedEventArgs const&)
{
    using namespace Microsoft::Windows::AppLifecycle;

    auto activationArgs = AppInstance::GetCurrent().GetActivatedEventArgs();

    // We'll register this as a reusable instance, and then
    // go ahead and do normal initialization.
    const winrt::hstring szKey = L"f5e06f90-bef6-53b2-9f26-6e5a341ce81a";
    auto keyInstance = AppInstance::FindOrRegisterForKey(szKey);

    // If we successfully registered the key, we must be the
    // only instance running that was activated for this key.
    if (!keyInstance.IsCurrent())
    {
        co_await keyInstance.RedirectActivationToAsync(activationArgs);

        Application::Current().Exit();
    }
    else
    {
        window = make<MainWindow>();
        window.Activate();
    }
}
