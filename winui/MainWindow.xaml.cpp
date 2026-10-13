#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <fstream>
#include <filesystem>
#include <commctrl.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;

namespace winrt::SyslogKit::implementation
{
    static hstring SeverityToString(int sev) {
        static const wchar_t* s[] = { L"Emerg", L"Alert", L"Crit", L"Error", L"Warn", L"Notice", L"Info", L"Debug" };
        return (sev >= 0 && sev <= 7) ? hstring(s[sev]) : L"Unk";
    }

    static LRESULT CALLBACK WindowSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (uMsg == WM_GETMINMAXINFO)
        {
            auto minMax = reinterpret_cast<MINMAXINFO*>(lParam);
            minMax->ptMinTrackSize.x = 850;
            minMax->ptMinTrackSize.y = 550;
            return 0;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    MainWindow::MainWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        SetWindowSubclass(GetWindowHandle(), WindowSubclassProc, 1, 0);

        m_dispatcherQueue = Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        m_liveLogs = single_threaded_observable_vector<winrt::SyslogKit::LogItem>();
        m_dbLogs = single_threaded_observable_vector<winrt::SyslogKit::LogItem>();

        auto localFolder = ApplicationData::Current().LocalFolder().Path();
        std::filesystem::path dbPath = std::filesystem::path(localFolder.c_str()) / "SyslogKit_Local.db";

        if (!m_storage.open(dbPath.string())) {
            ShowMessageDialog(L"Error", L"Failed to open default SQLite database!");
        } else {
            TxtCurrentDb().Text(L"DB: " + to_hstring(dbPath.filename().string()));
            ToolTipService::SetToolTip(TxtCurrentDb(), box_value(to_hstring(dbPath.string())));
        }

        m_server.set_callback([this](::SyslogKit::SyslogMessage const& msg) {
            m_storage.write(msg);
            m_dispatcherQueue.TryEnqueue([this, msg]() {
                ApplyLogMessage(msg);
            });
        });

        LoadSettings();

        NavView().SelectedItem(NavView().MenuItems().GetAt(0));
    }

    MainWindow::~MainWindow()
    {
        m_server.stop();
    }

    HWND MainWindow::GetWindowHandle()
    {
        HWND hwnd;
        this->try_as<IWindowNative>()->get_WindowHandle(&hwnd);
        return hwnd;
    }

    void MainWindow::ApplyLogMessage(::SyslogKit::SyslogMessage const& msg)
    {
        if (m_liveLogs.Size() > 5000) {
            m_liveLogs.RemoveAt(0);
        }

        auto item = make<LogItem>(
            to_hstring(msg.timestamp),
            static_cast<int32_t>(msg.facility),
            static_cast<int32_t>(msg.severity),
            SeverityToString(static_cast<int>(msg.severity)),
            to_hstring(msg.hostname),
            to_hstring(msg.app_name),
            to_hstring(msg.message)
        );

        m_liveLogs.Append(item);
        LiveListView().ScrollIntoView(item);
    }

    void MainWindow::OnToggleServerClicked(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_isRunning) {
            m_server.stop();
            TxtServerStatus().Text(L"Stopped");
            TxtServerStatus().Foreground(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 128, 128, 128)));
            BtnStartServer().Content(box_value(L"Start Server"));
            m_isRunning = false;
        } else {
            const int port = static_cast<int>(PortNumberBox().Value());
            const bool useUdp = ChkUdp().IsChecked().GetBoolean();
            const bool useTcp = ChkTcp().IsChecked().GetBoolean();

            if (!useUdp && !useTcp) {
                ShowMessageDialog(L"Configuration Error", L"Please enable at least one protocol (UDP or TCP).");
                return;
            }

            if (m_server.start(static_cast<uint16_t>(port), useUdp, useTcp)) {
                std::wstring protos = L"Running ";
                if (useUdp) protos += L"UDP";
                if (useUdp && useTcp) protos += L"/";
                if (useTcp) protos += L"TCP";
                protos += L" :" + std::to_wstring(port);

                TxtServerStatus().Text(protos);
                TxtServerStatus().Foreground(Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 102, 187, 106)));
                BtnStartServer().Content(box_value(L"Stop Server"));
                m_isRunning = true;
            } else {
                ShowMessageDialog(L"Socket Bind Error", L"Failed to start server on port " + to_hstring(port) + L". Port may already be in use.");
            }
        }
    }

    void MainWindow::OnClearLiveViewClicked(IInspectable const&, RoutedEventArgs const&)
    {
        m_liveLogs.Clear();
    }

    void MainWindow::OnRefreshDbClicked(IInspectable const&, RoutedEventArgs const&)
    {
        ::SyslogKit::LogFilter filter;
        filter.search_text = to_string(SearchBox().Text());

        auto selectedItem = LimitComboBox().SelectedItem().as<ComboBoxItem>();
        filter.limit = std::stoi(to_string(unbox_value<hstring>(selectedItem.Tag())));

        auto logs = m_storage.query(filter);
        m_dbLogs.Clear();
        for (const auto& msg : logs) {
            m_dbLogs.Append(make<LogItem>(
                to_hstring(msg.timestamp),
                static_cast<int32_t>(msg.facility),
                static_cast<int32_t>(msg.severity),
                SeverityToString(static_cast<int>(msg.severity)),
                to_hstring(msg.hostname),
                to_hstring(msg.app_name),
                to_hstring(msg.message)
            ));
        }
    }

    void MainWindow::OnSearchKeyDown(IInspectable const&, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e)
    {
        if (e.Key() == Windows::System::VirtualKey::Enter) {
            OnRefreshDbClicked(nullptr, nullptr);
        }
    }

    void MainWindow::OnLimitSelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (m_storage.is_open()) {
            OnRefreshDbClicked(nullptr, nullptr);
        }
    }

    void MainWindow::OnNavSelectionChanged(NavigationView const&, NavigationViewSelectionChangedEventArgs const& args)
    {
        if (args.IsSettingsSelected()) {
            LiveTabContent().Visibility(Visibility::Collapsed);
            DbTabContent().Visibility(Visibility::Collapsed);
            SettingsTabContent().Visibility(Visibility::Visible);
            return;
        }

        auto item = args.SelectedItem().as<NavigationViewItem>();
        auto tag = unbox_value<hstring>(item.Tag());

        LiveTabContent().Visibility(tag == L"live" ? Visibility::Visible : Visibility::Collapsed);
        DbTabContent().Visibility(tag == L"db" ? Visibility::Visible : Visibility::Collapsed);
        SettingsTabContent().Visibility(tag == L"settings" ? Visibility::Visible : Visibility::Collapsed);

        if (tag == L"db") {
            OnRefreshDbClicked(nullptr, nullptr);
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::OnSwitchDbClicked(IInspectable const&, RoutedEventArgs const&)
    {
        FileOpenPicker picker;
        picker.as<IInitializeWithWindow>()->Initialize(GetWindowHandle());
        picker.FileTypeFilter().Append(L".db");
        picker.FileTypeFilter().Append(L".sqlite");
        picker.SuggestedStartLocation(PickerLocationId::DocumentsLibrary);

        auto file = co_await picker.PickSingleFileAsync();
        if (file) {
            std::string path = to_string(file.Path());
            if (m_storage.open(path)) {
                TxtCurrentDb().Text(L"DB: " + file.Name());
                ToolTipService::SetToolTip(TxtCurrentDb(), box_value(file.Path()));
                OnRefreshDbClicked(nullptr, nullptr);
                co_await ShowMessageDialog(L"Database", L"Switched to new database successfully.");
            } else {
                co_await ShowMessageDialog(L"Error", L"Failed to open database file.");
            }
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::OnExportDbClicked(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_storage.is_open()) {
            co_await ShowMessageDialog(L"Warning", L"No database is currently opened.");
            co_return;
        }

        FileSavePicker picker;
        picker.as<IInitializeWithWindow>()->Initialize(GetWindowHandle());
        picker.FileTypeChoices().Insert(L"SQLite Database", single_threaded_vector<hstring>({ L".db" }));
        picker.SuggestedFileName(L"backup.db");

        auto destFile = co_await picker.PickSaveFileAsync();
        if (destFile) {
            std::filesystem::path src = m_storage.get_db_path();
            std::filesystem::path dst = destFile.Path().c_str();
            std::error_code ec;
            std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);

            if (!ec) {
                co_await ShowMessageDialog(L"Success", L"Database exported successfully.");
            } else {
                co_await ShowMessageDialog(L"Error", L"Failed to copy file. Check permissions.");
            }
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::OnExportLogsClicked(IInspectable const&, RoutedEventArgs const&)
    {
        FileSavePicker picker;
        picker.as<IInitializeWithWindow>()->Initialize(GetWindowHandle());
        picker.FileTypeChoices().Insert(L"Log File", single_threaded_vector<hstring>({ L".log" }));
        picker.SuggestedFileName(L"syslog_export.log");

        auto file = co_await picker.PickSaveFileAsync();
        if (file) {
            std::ofstream out(to_string(file.Path()));
            if (out.is_open()) {
                for (auto const& item : m_dbLogs) {
                    out << to_string(item.Timestamp()) << "\t"
                        << to_string(item.Hostname()) << "\t"
                        << to_string(item.Message()) << "\n";
                }
                co_await ShowMessageDialog(L"Success", L"Logs exported successfully.");
            } else {
                co_await ShowMessageDialog(L"Error", L"Could not open destination file for writing.");
            }
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::OnClearDbClicked(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_storage.is_open()) co_return;

        ContentDialog dialog;
        dialog.XamlRoot(Content().XamlRoot());
        dialog.Title(box_value(L"Clear Database"));
        dialog.PrimaryButtonText(L"Delete All");
        dialog.SecondaryButtonText(L"Older than 7 days");
        dialog.CloseButtonText(L"Cancel");
        dialog.Content(box_value(L"Select log retention strategy to delete records:"));

        auto result = co_await dialog.ShowAsync();
        if (result == ContentDialogResult::Primary) {
            m_storage.clear_database(0);
        } else if (result == ContentDialogResult::Secondary) {
            m_storage.clear_database(7);
        } else {
            co_return;
        }

        co_await ShowMessageDialog(L"Success", L"Database cleanup completed.");
        OnRefreshDbClicked(nullptr, nullptr);
    }

    void MainWindow::OnLogDoubleTapped(IInspectable const& sender, Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const&)
    {
        auto listView = sender.as<ListView>();
        auto selected = listView.SelectedItem();
        if (selected) {
            ShowDetailDialog(selected.as<winrt::SyslogKit::LogItem>());
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::ShowDetailDialog(SyslogKit::LogItem const& item)
    {
        ContentDialog dialog;
        dialog.XamlRoot(Content().XamlRoot());
        dialog.Title(box_value(L"Log Details"));
        dialog.CloseButtonText(L"Close");

        StackPanel panel;
        panel.Spacing(8);

        auto addField = [&](hstring const& label, hstring const& val) {
            TextBox tb;
            tb.Header(box_value(label));
            tb.Text(val);
            tb.IsReadOnly(true);
            panel.Children().Append(tb);
        };

        addField(L"Timestamp", item.Timestamp());
        addField(L"Hostname", item.Hostname());
        addField(L"Application", item.AppName());
        addField(L"Facility", to_hstring(item.Facility()));
        addField(L"Severity", to_hstring(item.Severity()) + L" (" + item.SeverityName() + L")");

        TextBox msgBox;
        msgBox.Header(box_value(L"Message"));
        msgBox.Text(item.Message());
        msgBox.IsReadOnly(true);
        msgBox.TextWrapping(TextWrapping::Wrap);
        msgBox.AcceptsReturn(true);
        msgBox.MaxHeight(150);
        panel.Children().Append(msgBox);

        dialog.Content(panel);
        co_await dialog.ShowAsync();
    }

    void MainWindow::LoadSettings()
    {
        auto values = ApplicationData::Current().LocalSettings().Values();
        if (values.HasKey(L"Port")) {
            PortNumberBox().Value(unbox_value<int32_t>(values.Lookup(L"Port")));
        }
        if (values.HasKey(L"Udp")) {
            ChkUdp().IsChecked(unbox_value<bool>(values.Lookup(L"Udp")));
        }
        if (values.HasKey(L"Tcp")) {
            ChkTcp().IsChecked(unbox_value<bool>(values.Lookup(L"Tcp")));
        }
    }

    void MainWindow::OnSaveSettingsClicked(IInspectable const&, RoutedEventArgs const&)
    {
        auto values = ApplicationData::Current().LocalSettings().Values();
        values.Insert(L"Port", box_value(static_cast<int32_t>(PortNumberBox().Value())));
        values.Insert(L"Udp", box_value(ChkUdp().IsChecked().GetBoolean()));
        values.Insert(L"Tcp", box_value(ChkTcp().IsChecked().GetBoolean()));

        ShowMessageDialog(L"Settings", L"Settings saved. Restart the server to apply network changes.");
    }

    void MainWindow::OnRestoreDefaultsClicked(IInspectable const&, RoutedEventArgs const&)
    {
        PortNumberBox().Value(5140);
        ChkUdp().IsChecked(true);
        ChkTcp().IsChecked(true);
        DefaultLimitCombo().SelectedIndex(1);
        OnSaveSettingsClicked(nullptr, nullptr);
    }

    Windows::Foundation::IAsyncAction MainWindow::ShowMessageDialog(hstring const& title, hstring const& content)
    {
        ContentDialog dialog;
        dialog.XamlRoot(Content().XamlRoot());
        dialog.Title(box_value(title));
        dialog.Content(box_value(content));
        dialog.CloseButtonText(L"OK");
        co_await dialog.ShowAsync();
    }
}