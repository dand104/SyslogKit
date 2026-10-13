#pragma once
#include "MainWindow.g.h"
#include "LogItem.h"
#include "SyslogKit/SyslogServer.hxx"
#include "SyslogKit/LogStorage.hxx"
#include <winrt/Microsoft.UI.Dispatching.h>

namespace winrt::SyslogKit::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();

        Windows::Foundation::Collections::IObservableVector<winrt::SyslogKit::LogItem> LiveLogs() const { return m_liveLogs; }
        Windows::Foundation::Collections::IObservableVector<winrt::SyslogKit::LogItem> DbLogs() const { return m_dbLogs; }

        void OnNavSelectionChanged(Microsoft::UI::Xaml::Controls::NavigationView const&, Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const&);
        void OnToggleServerClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnClearLiveViewClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnLogDoubleTapped(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const&);

        Windows::Foundation::IAsyncAction OnSwitchDbClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        Windows::Foundation::IAsyncAction OnExportDbClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        Windows::Foundation::IAsyncAction OnClearDbClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        Windows::Foundation::IAsyncAction OnExportLogsClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        void OnRefreshDbClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnSearchKeyDown(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const&);
        void OnLimitSelectionChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnSaveSettingsClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnRestoreDefaultsClicked(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

    private:
        HWND GetWindowHandle();
        void LoadSettings();
        void ApplyLogMessage(::SyslogKit::SyslogMessage const& msg);
        Windows::Foundation::IAsyncAction ShowMessageDialog(hstring const& title, hstring const& content);
        Windows::Foundation::IAsyncAction ShowDetailDialog(winrt::SyslogKit::LogItem const& item);

        ::SyslogKit::Server m_server;
        ::SyslogKit::LogStorage m_storage;
        bool m_isRunning{ false };
        Microsoft::UI::Dispatching::DispatcherQueue m_dispatcherQueue{ nullptr };

        Windows::Foundation::Collections::IObservableVector<winrt::SyslogKit::LogItem> m_liveLogs;
        Windows::Foundation::Collections::IObservableVector<winrt::SyslogKit::LogItem> m_dbLogs;
    };
}

namespace winrt::SyslogKit::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}