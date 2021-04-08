#include "influxdb.hh"

#include <iostream>
#include <sstream>

#include <ctime>

#include <cpr/cpr.h>

#include "common/log.hh"

namespace ganymede::influx {

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace {

std::ostream& operator<<(std::ostream& os, const value& value)
{
    std::visit(overloaded {
        [&os](int i) { os << i << "i"; },
        [&os](const std::string& s) { os << '"' << s << '"'; },
        [&os](double d) { os << std::fixed << d; },
    }, value);

    return os;
}

}

Point::Point(std::string&& series, std::vector<tag_pair>&& tags, std::vector<value_pair>&& data, const std::chrono::time_point<std::chrono::high_resolution_clock>& now)
    : m_series(std::move(series))
    , m_tags(std::move(tags))
    , m_values(std::move(data))
    , m_time(now)
{
}

Point& Point::SetSeries(const std::string& series)
{
    m_series = series;
    return *this;
}

Point& Point::AddTag(tag_pair&& tag)
{
    m_tags.emplace_back(std::move(tag));
    return *this;
}

Point& Point::AddValue(value_pair&& value)
{
    m_values.emplace_back(std::move(value));
    return *this;
}

Point& Point::SetTime(const std::chrono::time_point<std::chrono::high_resolution_clock>& time)
{
    m_time = time;
    return *this;
}

std::ostream& operator<<(std::ostream& os, const Point& p)
{
    bool first_value = true;
    os << p.m_series;

    for (const tag_pair& tag : p.m_tags) {
        os << ',' << tag.first << '=' << tag.second;
    }
    os << ' ';

    for (const value_pair& point : p.m_values) {
        if (not first_value) {
            os << ',';
        }
        os << point.first << '=' << point.second;
        first_value = false;
    }

    os << ' ' << std::chrono::duration_cast<std::chrono::nanoseconds>(p.m_time.time_since_epoch()).count();
    return os;
}

InfluxDB::InfluxDB(const std::string& host, const std::string& org, const std::string& bucket, const std::string& token)
    : m_host(host)
    , m_org(org)
    , m_bucket(bucket)
    , m_token(token)
    , m_batch_size(1)
{
}

InfluxDB::~InfluxDB()
{
    Flush();
}

void InfluxDB::Write(std::vector<Point> points)
{
    std::ostringstream os;

    for (const Point& point : points) {
        if (point.empty()) {
            continue;
        }

        os << point << std::endl;
        m_lines.push_back(os.str());
        os.clear();
    }

    if (m_lines.size() >= m_batch_size) {
        Flush();
    }
}

void InfluxDB::Flush()
{
    if (m_lines.size() == 0) {
        return;
    }
    std::ostringstream body;

    for (const std::string& line : m_lines) {
        body << line << std::endl;
    }

    cpr::Response response = cpr::Post(
        cpr::Url{m_host + "/api/v2/write"},
        cpr::Header{{"Authorization", "Token " + m_token}},
        cpr::Parameters{{"org", std::string(m_org)}, {"bucket", std::string(m_bucket)}, {"precision", "ns"}},
        cpr::Body{body.str()}
    );

    if (response.status_code / 200 != 1) {
        nlohmann::json error = {{"message", "failed to write data to InfluxDB"}, {"local_error", response.error.message}, {"influx_status", response.status_code}};
        try {
            error["influx_error"] = nlohmann::json::parse(response.text);
        } catch (...) { }
        common::log::error(error);
    }

    if (response.status_code / 200 == 1 or response.status_code / 400 == 1) {
        m_lines.clear();
    }
}

std::vector<Point> InfluxDB::Query(const std::string& flux_query)
{
    // FIXME: This function is absolute garbage. The response parsing is
    // terrible, and only works for one specific query!
    cpr::Response response = cpr::Post(
        cpr::Url{m_host + "/api/v2/query"},
        cpr::Header{{"Authorization", "Token " + m_token}, {"Content-Type", "application/vnd.flux"}},
        cpr::Parameters{{"org", std::string(m_org)}, {"bucket", std::string(m_bucket)}, {"precision", "ns"}},
        cpr::Body{flux_query}
    );

    if (response.status_code / 200 != 1) {
                nlohmann::json error = {{"message", "failed to write data to InfluxDB"}, {"local_error", response.error.message}, {"influx_status", response.status_code}};
        try {
            error["influx_error"] = nlohmann::json::parse(response.text);
        } catch (...) { }
        common::log::error(error);
        return {};
    }

    std::vector<Point> points;
    std::string line, token;
    std::istringstream body(response.text);

    std::ofstream("influx.data") << response.text;

    // Ignore first line of headers
    std::getline(body, line, '\n');
    while (std::getline(body, line, '\n')) {
        int token_no = 0;
        std::istringstream line_stream(line);

        // Ignore first elements because they are static in our case
        while ((token_no++ <= 5) && std::getline(line_stream, token, ','));

        Point point;
        {
            struct tm time_tm;
            strptime(token.c_str(), "%Y-%m-%dT%TZ", &time_tm);
            time_t c_time = mktime(&time_tm);
            std::chrono::time_point<std::chrono::high_resolution_clock> time = std::chrono::time_point<std::chrono::high_resolution_clock>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(c_time)));
            point.SetTime(time);
        }
        {
            std::string value, field;
            std::getline(line_stream, value, ',');
            std::getline(line_stream, field, ',');
            point.AddValue({field, std::atof(value.c_str())});
        }
        {
            std::getline(line_stream, token, ',');
            point.SetSeries(std::string(token));
            std::getline(line_stream, token, ',');
            point.AddTag({"domain", token});
            std::getline(line_stream, token, '\r');
            point.AddTag({"source", token});
        }

        points.push_back(point);
    }

    return points;
}


}
