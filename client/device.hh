#ifndef DEVICE_HH
#define DEVICE_HH

#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <random>
#include <chrono>

class Device {
public:
    Device(std::string device_name);
    ~Device();

    void enable_temperature(bool enable);
    void enable_weight(bool enable);

    void exit();

    void initialize();

    void set_speed_frequency(int freq);
    void set_weight_frequency(int freq);
    void set_temperature_frequency(int freq);

private:

    void send_telemetry_data();

    void speed_sensor_update();
    void weight_sensor_update();
    void temperature_sensor_update();

    int random_val(int min_num, int max_num);
    
    std::condition_variable update_ready;

    std::string name;
    std::atomic<int> speed_freq;
    std::atomic<int> weight_freq;
    std::atomic<int> temp_freq;

    int speed;
    int weight;
    int temperature;

    std::thread telemetry_thread;
    std::thread speed_thread;
    std::thread weight_thread;
    std::thread temp_thread;

    std::atomic<bool> temp_enabled;
    std::atomic<bool> weight_enabled;

    std::atomic<bool> exit_thread;


    std::mutex device_mutex;
};

#endif // DEVICE_HH