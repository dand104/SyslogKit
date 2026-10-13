#include "pch.h"
#include "LogItem.h"
#if __has_include("LogItem.g.cpp")
#include "LogItem.g.cpp"
#endif

using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace winrt::SyslogKit::implementation
{
    LogItem::LogItem(hstring const& timestamp, int32_t facility, int32_t severity, hstring const& severityName,
                     hstring const& hostname, hstring const& appName, hstring const& message)
        : m_timestamp(timestamp), m_facility(facility), m_severity(severity),
          m_severityName(severityName), m_hostname(hostname), m_appName(appName), m_message(message)
    {
        if (severity <= 3) {
            m_brush = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 239, 83, 80));
        } else if (severity == 4) {
            m_brush = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 255, 167, 38));
        } else {
            m_brush = SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 220, 220, 220));
        }
    }
}