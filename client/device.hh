#ifndef DEVICE_HH
#define DEVICE_HH

#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <iostream>
#include <random>
#include <chrono>

class Device {
public:
    // Construct device with the given name; all optional sensors start disabled.
    Device(std::string device_name);

    ~Device();

    // Enable or disable the temperature sensor.
    void
    enable_temperature(bool enable);

    // Enable or disable the weight sensor.
    void
    enable_weight(bool enable);

    // Signal all threads to stop and join them.
    void
    exit();

    // Start all sensor and sender threads.
    void
    initialize();

    // Set the polling interval (seconds) for the speed sensor.
    void
    set_speed_frequency(int freq);

    // Set the polling interval (seconds) for the weight sensor.
    void
    set_weight_frequency(int freq);

    // Set the polling interval (seconds) for the temperature sensor.
    void
    set_temperature_frequency(int freq);

private:
    // Drain send_queue and POST batches to the server.
    void
    sender_loop();

    // Poll speed, push changed readings to send_queue.
    void
    speed_sensor_update();

    // Poll weight, push changed readings to send_queue.
    void
    weight_sensor_update();

    // Poll temperature, push changed readings to send_queue.
    void
    temperature_sensor_update();

    // Return a random integer in [min_num, max_num].
    int
    random_val(int min_num, int max_num);

    // Device name — used as the sensor_id prefix.
    std::string name;

    // Polling intervals in seconds for each sensor.
    std::atomic<int> speed_freq;
    std::atomic<int> weight_freq;
    std::atomic<int> temp_freq;

    // Most-recently observed sensor values (protected by device_mutex).
    int speed;
    int weight;
    int temperature;

    // Worker threads.
    std::thread sender_thread;
    std::thread speed_thread;
    std::thread weight_thread;
    std::thread temp_thread;

    // Flags to enable optional sensors.
    std::atomic<bool> temp_enabled;
    std::atomic<bool> weight_enabled;

    // Set to true to ask all threads to terminate.
    std::atomic<bool> exit_thread;

    // Guards speed, weight, temperature.
    std::mutex device_mutex;

    // Serialised JSON strings ready to POST; sensor threads push, sender_loop drains.
    std::queue<std::string> send_queue;

    // Guards send_queue.
    std::mutex queue_mutex;

    // Notified when send_queue is non-empty or exit_thread is set.
    std::condition_variable queue_cv;
};

#endif // DEVICE_HH
