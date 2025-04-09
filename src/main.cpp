#include <iostream>
#include <vector>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include "time_meas.hpp"


#define ENABLE_CAMERA true


/**
 * Extract contours from a frame.
 *
 * @param frame Input frame from the camera.
 * @return Contours of the input frame.
 */
cv::Mat contour_frame(const cv::Mat& frame) {
    cv::Mat gray, blur, canny;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);            // Convert to grayscale
    cv::GaussianBlur(gray, blur, cv::Size(5, 5), 0);          // Apply Gaussian blur
    cv::Canny(blur, canny, 50, 150);                          // Canny edge detection
    return canny;
}

/**
 * Perform monocular depth estimation.
 *
 * @param frame Input frame from the camera.
 * @return Depth map of the input frame.
 */
cv::Mat depth_estimation(cv::Mat frame) {
    cv::Mat input;
    cv::resize(frame, input, cv::Size(256, 256));

    cv::Mat blob = cv::dnn::blobFromImage(input, 1.0 / 255.0, cv::Size(256, 256), cv::Scalar(0, 0, 0), true, false);

    // Load the MiDaS model
    static cv::dnn::Net net;

    if (net.empty()) {
        try {
            // Try to load MiDaS small model
            // Download this file from: https://github.com/isl-org/MiDaS/releases/download/v2_1/model-small.onnx
            // and put in models directory.
            net = cv::dnn::readNetFromONNX("../models/model-small.onnx");
            // Download DINO-v2 from https://github.com/fabio-sim/Depth-Anything-ONNX/releases
//            net = cv::dnn::readNetFromONNX("../models/depth_anything_v2_vits.onnx");

//            std::cout << "Model loaded successfully!" << std::endl;
        } catch (const cv::Exception& e) {
            std::cerr << "Error loading model: " << e.what() << std::endl;
            // Return original frame if model can't be loaded
            return frame;
        }
    }

    net.setInput(blob);
    cv::Mat output = net.forward();

    // Post-processing
    // Convert the output to a displayable depth map
    cv::Mat depth_map;
    cv::Mat resized_output;

    // Resize output to input frame dimensions
    auto* output_data = (float*)output.data;
    cv::Mat result(output.size[1], output.size[2], CV_32F, output_data);

    // Normalize the depth map for better visualization
    cv::normalize(result, result, 0, 1, cv::NORM_MINMAX);

    // Resize to original frame size
    cv::resize(result, depth_map, cv::Size(frame.cols, frame.rows));

    // Convert to 8-bit for display and apply colormap
    depth_map.convertTo(depth_map, CV_8UC1, 255);
    cv::Mat colored_depth_map;
    cv::applyColorMap(depth_map, colored_depth_map, cv::COLORMAP_INFERNO);

    return colored_depth_map;
}

int main() {
#if ENABLE_CAMERA
    cv::Mat video_from_facecam;
    cv::VideoCapture cap(0);

    if (!cap.isOpened()) {
        std::cout << "Could not open video stream!" << std::endl;
        return -1;
    }

    std::cout << "Press 'q' to quit" << std::endl;

    std::vector<long long> depth_times_ms;
    std::vector<double> fps_values;

    while (char(cv::waitKey(1)) != 'q') {
        auto start_time = get_current_time_fenced();

        cap >> video_from_facecam;
        if (video_from_facecam.empty()) {
            break;
        }

        auto depth_start_time = get_current_time_fenced();
        cv::Mat depth_map = depth_estimation(video_from_facecam);
        auto depth_end_time = get_current_time_fenced();

        // Measure depth estimation time
        auto depth_time = to_ms(depth_end_time - depth_start_time);
        depth_times_ms.push_back(depth_time);

        // Calculate FPS for this frame
        if (depth_time > 0) {
            double fps = 1000.0 / static_cast<double>(depth_time);
            fps_values.push_back(fps);
        }

        imshow("Depth Estimation", depth_map);
    }

    cap.release();

    // Compute average depth time
    if (!depth_times_ms.empty()) {
        double avg_depth_time = std::accumulate(depth_times_ms.begin(), depth_times_ms.end(), 0.0) / depth_times_ms.size();
        std::cout << "Average depth estimation time: " << avg_depth_time << " ms" << std::endl;
        // Average depth estimation time: 40.87 ms
    }

    // Compute average FPS
    if (!fps_values.empty()) {
        double avg_fps = std::accumulate(fps_values.begin(), fps_values.end(), 0.0) / static_cast<double>(fps_values.size());
        std::cout << "Average FPS: " << avg_fps << std::endl;
        // Average FPS: 21.2945
    }

#else
    std::string image_path = "../media/test_image_1.png";
    cv::Mat image = cv::imread(image_path);

    if (image.empty()) {
        std::cout << "Could not open or find the image!" << std::endl;
        return -1;
    }

    std::cout << "Performing depth estimation on the image..." << std::endl;
    cv::Mat depth_map = depth_estimation(image);

    std::string output_path = "../media/results/test_image_1.jpg";
    cv::imwrite(output_path, depth_map);

    std::cout << "Depth map saved to " << output_path << std::endl;
#endif

    cv::destroyAllWindows();
    return 0;
}
