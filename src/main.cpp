#include <iostream>
#include <sm_screenshotter.hpp>

int main(int argument_count, char** arguments) {

    sm_screenshotter_t s{"../assets/", "../assets/Enjoy The Show/Enjoy The Show.ssc"};
    if (!s) {
        std::cout << "Failed to load all assets..." << std::endl;
    }
    if (!s.save("chart.bmp")) {
        std::cout << "Failed to save bmp..." << std::endl;
    }
    return 0;
}