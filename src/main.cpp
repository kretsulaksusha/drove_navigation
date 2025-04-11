#include "video_processor.hpp"

int main(int argc, char** argv) {
    std::string video_filename = (argc > 1) ? argv[1] : "helicopter.mp4";
    std::string video_path = getContentPath(video_filename);

    selectROI(video_path);

    return 0;
}
