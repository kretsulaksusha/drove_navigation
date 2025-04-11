#include <iostream>
#include "feature_detector.hpp"

int main(int argc, char** argv) {
    std::string image_filename = (argc > 1) ? argv[1] : "test_image_4.png";
    std::string image_path = getContentPath(image_filename);

    test_fast_detector(image_path);

    return 0;
}
