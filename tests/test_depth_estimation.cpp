#include "depth_estimation.hpp"
#include "model_types.hpp"


int main(int argc, char** argv) {
    std::string image_filename = (argc > 1) ? argv[1] : "test_image_1.png";
    std::string image_path = getContentPath(image_filename);

    int enable_webcam = (argc > 2) ? std::stoi(argv[2]) : 0;
    std::string model = (argc > 3) ? argv[3] : "dad"; // "dad", "dav2" or "midas"
    ModelType model_type = stringToModelType(model);

    // if the second parameter is true -> test with webcam, false -> for static image,
    test_depth_estimation(image_path, enable_webcam, model_type);

    return 0;
}
