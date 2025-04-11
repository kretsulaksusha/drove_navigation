#include "depth_estimation.hpp"

int main(int argc, char** argv) {
    std::string image_filename = (argc > 1) ? argv[1] : "test_image_1.png";
    std::string image_path = getContentPath(image_filename);

    // if second parameter is true -> test with webcam, false -> for static image
    test_depth_estimation(image_path, false);

    return 0;
}
