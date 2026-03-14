#include "device.hh"
#include "constants.hh"
#include "../deps/httplib.h"
#include "../deps/json.hpp"
#include <cstdlib>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

static void parse_server_url(std::string& host, int& port) {
    host = constants::SERVER_DEFAULT_HOST;
    port = constants::SERVER_DEFAULT_PORT;
    const char* url = std::getenv("SERVER_URL");
    if (!url) return;
    std::string s(url);
    auto pfx = s.find("://");
    if (pfx != std::string::npos) s = s.substr(pfx + 3);
    auto colon = s.rfind(':');
    if (colon != std::string::npos) {
        host = s.substr(0, colon);
        port = std::stoi(s.substr(colon + 1));
    } else {
        host = s;
    }
}

static std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

static json make_reading(const std::string& device_name, const std::string& sensor_type,
                         int value, const std::string& unit, const std::string& ts) {
    return {{"sensor_id", device_name + "_" + sensor_type},
            {"sensor_type", sensor_type}, {"device_name", device_name},
            {"value", value}, {"unit", unit}, {"timestamp", ts}};
}

Device::Device(std::string device_name)
    : name(device_name),
      speed_freq(constants::DEFAULT_SENSOR_FREQ),
      weight_freq(constants::DEFAULT_SENSOR_FREQ),
      temp_freq(constants::DEFAULT_SENSOR_FREQ),
      speed(constants::SPEED_INITIAL),
      weight(constants::WEIGHT_INITIAL),
      temperature(constants::TEMP_INITIAL),
      temp_enabled(false),
      weight_enabled(false),
      exit_thread(false)
{
}

Device::~Device()
{
    if (!exit_thread.load())
        exit();
}

void
Device::exit()
{
    exit_thread.store(true);

    queue_cv.notify_all();

    if (sender_thread.joinable())
        sender_thread.join();

    if (speed_thread.joinable())
        speed_thread.join();

    if (weight_thread.joinable())
        weight_thread.join();

    if (temp_thread.joinable())
        temp_thread.join();
}

void
Device::initialize()
{
    sender_thread = std::thread(&Device::sender_loop, this);
    speed_thread   = std::thread(&Device::speed_sensor_update, this);
    weight_thread  = std::thread(&Device::weight_sensor_update, this);
    temp_thread    = std::thread(&Device::temperature_sensor_update, this);
}


void
Device::set_speed_frequency(int freq)
{
    speed_freq.store(freq);
}

void
Device::set_weight_frequency(int freq)
{
    weight_freq.store(freq);
}

void
Device::set_temperature_frequency(int freq)
{
    temp_freq.store(freq);
}

void
Device::enable_temperature(bool enable)
{
    temp_enabled.store(enable);
}

void
Device::enable_weight(bool enable)
{
    weight_enabled.store(enable);
}


void
Device::sender_loop()
{
    std::string host;
    int port;
    parse_server_url(host, port);

    while (!exit_thread.load())
    {
        std::vector<std::string> batch;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] {
                return !send_queue.empty() || exit_thread.load();
            });
            while (!send_queue.empty()) {
                batch.push_back(std::move(send_queue.front()));
                send_queue.pop();
            }
        }

        if (batch.empty()) continue;

        // Build JSON array from pre-serialised strings
        std::string payload = "[";
        for (size_t i = 0; i < batch.size(); ++i) {
            if (i) payload += ",";
            payload += batch[i];
        }
        payload += "]";

        std::cout << "Sending " << batch.size() << " reading(s)" << std::endl;

        httplib::Client cli(host, port);
        cli.set_connection_timeout(constants::CONNECTION_TIMEOUT_SEC);
        auto res = cli.Post(constants::SERVER_INGEST_PATH, payload, constants::CONTENT_TYPE_JSON);
        if (!res || res->status != 201)
            std::cerr << "Failed to post batch (" << batch.size() << " reading(s))" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(constants::SENDER_SLEEP_MS));
    }
}


void
Device::speed_sensor_update()
{
    while (!exit_thread.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(speed_freq));

        int new_val = random_val(constants::SPEED_MIN, constants::SPEED_MAX);
        {
            std::lock_guard<std::mutex> lock(device_mutex);
            if (new_val == speed) continue;
            speed = new_val;
        }

        std::string entry = make_reading(name, constants::SPEED_TYPE, new_val,
                                         constants::SPEED_UNIT, current_timestamp()).dump();
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            send_queue.push(std::move(entry));
        }
        queue_cv.notify_one();
    }
}


void
Device::weight_sensor_update()
{
    while (!exit_thread.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(weight_freq));

        if (!weight_enabled.load())
            continue;

        int new_val = random_val(constants::WEIGHT_MIN, constants::WEIGHT_MAX);
        {
            std::lock_guard<std::mutex> lock(device_mutex);
            if (new_val == weight) continue;
            weight = new_val;
        }

        std::string entry = make_reading(name, constants::WEIGHT_TYPE, new_val,
                                         constants::WEIGHT_UNIT, current_timestamp()).dump();
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            send_queue.push(std::move(entry));
        }
        queue_cv.notify_one();
    }
}


void
Device::temperature_sensor_update()
{
    while (!exit_thread.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(temp_freq));

        if (!temp_enabled.load())
            continue;

        int new_val = random_val(constants::TEMP_MIN, constants::TEMP_MAX);
        {
            std::lock_guard<std::mutex> lock(device_mutex);
            if (new_val == temperature) continue;
            temperature = new_val;
        }

        std::string entry = make_reading(name, constants::TEMP_TYPE, new_val,
                                         constants::TEMP_UNIT, current_timestamp()).dump();
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            send_queue.push(std::move(entry));
        }
        queue_cv.notify_one();
    }
}


// Copied from google search for generating random numbers in C++
int
Device::random_val(int min_num, int max_num)
{
    // Use a time-based seed for the random number engine
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 random_seed(seed); // Mersenne Twister 19937 generator

    // Define the uniform integer distribution
    std::uniform_int_distribution<int> dist(min_num, max_num);

    return dist(random_seed);
}
