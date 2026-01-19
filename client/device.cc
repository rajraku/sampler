#include "device.hh"

Device::Device(std::string device_name)
    : name(device_name),
      speed_freq(1),
      weight_freq(1),
      temp_freq(1),
      speed(26),
      weight(6000),
      temperature(24),
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

    // Notify all threads to exit..
    update_ready.notify_all();

    if (telemetry_thread.joinable())
        telemetry_thread.join();

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
    // Thread initialization..
    // Start telemetry data sending thread..
    telemetry_thread = std::thread(&Device::send_telemetry_data, this);

    // Start Speed sensor thread..
    speed_thread = std::thread(&Device::speed_sensor_update, this);    

    // Start Weight sensor thread..
    weight_thread = std::thread(&Device::weight_sensor_update, this);

    // Start Temperature sensor thread..
    temp_thread = std::thread(&Device::temperature_sensor_update, this);

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
Device::send_telemetry_data()
{
    while (!exit_thread.load())
    {
        {
            // Wait for update condition..
            std::unique_lock<std::mutex> lock(device_mutex);
            update_ready.wait(lock);
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Send telemetry data logic..
        std::cout << "Sending telemetry data: "
                  << name << " - "
                  << "Speed: " << speed
                  << ", Weight: " << weight
                  << ", Temperature: " << temperature
                  << std::endl;
    }
    
}


void
Device::speed_sensor_update()
{
    int min_num = 25;
    int max_num = 45;

    while (!exit_thread.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(speed_freq));
        
        speed = random_val(min_num, max_num);

        update_ready.notify_one();
    }
}


void
Device::weight_sensor_update()
{
    int min_num = 6000;
    int max_num = 6005;

    while (!exit_thread.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(weight_freq));

        if (!weight_enabled.load())
            continue;
        
        weight = random_val(min_num, max_num);

        update_ready.notify_one();
    }
}


void
Device::temperature_sensor_update()
{
    int min_num = 20;
    int max_num = 30;

    while (!exit_thread.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(temp_freq));

        if (!temp_enabled.load())
            continue;

        temperature = random_val(min_num, max_num);

        update_ready.notify_one();
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

