#include <iostream>
#include "device.hh"

int main() {
    std::cout << "Client application started." << std::endl;


    std::cout << "Enter the device name::";
    std::string device_name;
    std::cin >> device_name;

    Device device(device_name);
    device.initialize();

    while (true)
    {
        std::cout << "Enter command (set_speed_freq <value>, set_weight_freq <value>, set_temp_freq <value>, enable_temp <0/1>, enable_weight <0/1>, exit): ";
        std::string command;
        std::cin >> command;

        if (command == "exit") {
            device.exit();
            break;
        }

        int value;
        if (command == "set_speed_freq" || command == "set_weight_freq" || 
            command == "set_temp_freq" || command == "enable_temp" || 
            command == "enable_weight") {
            std::cin >> value;
        }

        if (command == "set_speed_freq") {
            device.set_speed_frequency(value);
        } else if (command == "set_weight_freq") {
            device.set_weight_frequency(value);
        } else if (command == "set_temp_freq") {
            device.set_temperature_frequency(value);
        } else if (command == "enable_temp") {
            device.enable_temperature(value != 0);
        } else if (command == "enable_weight") {
            device.enable_weight(value != 0);
        } else {
            std::cout << "Unknown command." << std::endl;
        }
    }
    


    
    return 0;
}