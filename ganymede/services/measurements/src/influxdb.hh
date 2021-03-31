#ifndef GANYMEDE_SERVICES_MEASUREMENTS_INFLUXDB_HH_
#define GANYMEDE_SERVICES_MEASUREMENTS_INFLUXDB_HH_

#include <chrono>
#include <string>
#include <variant>
#include <vector>
#include <deque>

namespace ganymede::influx {

using value = std::variant<int, std::string, double>;
using value_pair = std::pair<std::string, value>;
using tag_pair = std::pair<std::string, std::string>;

class Point {
public:
    Point() = default;
    Point(std::string&& series, std::vector<tag_pair>&& tags = {}, std::vector<value_pair>&& data = {}, const std::chrono::time_point<std::chrono::high_resolution_clock>& now = std::chrono::high_resolution_clock::now());

    Point& SetSeries(const std::string& series);
    Point& AddTag(tag_pair&& tag);
    Point& AddValue(value_pair&& value);
    Point& SetTime(const std::chrono::time_point<std::chrono::high_resolution_clock>& time);

    const std::string& series() const { return m_series; }
    const std::vector<value_pair>& values() const { return m_values; }
    const std::chrono::time_point<std::chrono::high_resolution_clock>& time() const { return m_time; }

    friend std::ostream& operator<<(std::ostream& os, const Point& p);
private:
    std::string m_series;
    std::vector<tag_pair> m_tags;
    std::vector<value_pair> m_values;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;
};

class InfluxDB {
public:
    InfluxDB() = delete;
    InfluxDB(const std::string& host, const std::string& org, const std::string& bucket, const std::string& token);
    ~InfluxDB();

    void SetBatchSize(std::size_t batch_size) { m_batch_size = batch_size; }
    void Write(std::vector<Point> points);
    std::vector<Point> Query(const std::string& flux_query);
    void Flush();

private:
    std::string m_host;
    std::string m_org;
    std::string m_bucket;
    std::string m_token;

    std::size_t m_batch_size;
    std::deque<std::string> m_lines;
};

}

#endif