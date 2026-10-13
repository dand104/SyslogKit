#pragma once
#include "LogItem.g.h"

namespace winrt::SyslogKit::implementation
{
    struct LogItem : LogItemT<LogItem>
    {
        LogItem() = default;
        LogItem(hstring const& timestamp, int32_t facility, int32_t severity, hstring const& severityName,
                hstring const& hostname, hstring const& appName, hstring const& message);

        hstring Timestamp() const { return m_timestamp; }
        int32_t Facility() const { return m_facility; }
        int32_t Severity() const { return m_severity; }
        hstring SeverityName() const { return m_severityName; }
        hstring Hostname() const { return m_hostname; }
        hstring AppName() const { return m_appName; }
        hstring Message() const { return m_message; }
        Microsoft::UI::Xaml::Media::SolidColorBrush SeverityBrush() const { return m_brush; }

    private:
        hstring m_timestamp;
        int32_t m_facility{ 0 };
        int32_t m_severity{ 0 };
        hstring m_severityName;
        hstring m_hostname;
        hstring m_appName;
        hstring m_message;
        Microsoft::UI::Xaml::Media::SolidColorBrush m_brush{ nullptr };
    };
}

namespace winrt::SyslogKit::factory_implementation
{
    struct LogItem : LogItemT<LogItem, implementation::LogItem> {};
}