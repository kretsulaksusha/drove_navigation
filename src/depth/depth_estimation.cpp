#include "depth_estimation.hpp"
#include "depth_estimator.hpp"


/**
 * Extract contours from a frame.
 *
 * @param frame Input frame from the camera.
 * @return Contours of the input frame.
 */
cv::Mat contour_frame(const cv::Mat& frame) {
    cv::Mat gray, blur, canny;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blur, cv::Size(5, 5), 0);
    cv::Canny(blur, canny, 50, 150);
    return canny;
}

/**
 * Test the depth estimation function.
 *
 * @param image_path Path to the input image.
 * @param enable_camera Enable camera input if true, otherwise use the image.
 * @param model_type Model type to use for depth estimation (DAV2, DAD, MiDaS).
 * @return 0 on success, non-zero on failure.
 */
int test_depth_estimation(std::string& image_path, bool enable_camera, ModelType model_type) {
    DepthEstimator depth_estimator(model_type);

    if (enable_camera) {
        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            std::cerr << "Could not open video stream!" << std::endl;
            return -1;
        }

        std::cout << "Press 'q' to quit" << std::endl;

        std::vector<long long> depth_times_ms;
        std::vector<double> fps_values;

        while (char(cv::waitKey(1)) != 'q') {
            cv::Mat frame;
            cap >> frame;
            if (frame.empty()) break;

            auto depth_start_time = get_current_time_fenced();
            cv::Mat depth_map = depth_estimator.infer(frame);
            auto depth_end_time = get_current_time_fenced();

            long long depth_time = to_ms(depth_end_time - depth_start_time);
            depth_times_ms.push_back(depth_time);

            if (depth_time > 0) {
                double fps = 1000.0 / static_cast<double>(depth_time);
                fps_values.push_back(fps);
            }

            cv::imshow("Depth Estimation", depth_map);
        }

        cap.release();

        if (!depth_times_ms.empty()) {
            double avg_time = std::accumulate(depth_times_ms.begin(), depth_times_ms.end(), 0.0) / depth_times_ms.size();
            std::cout << "Average depth estimation time: " << avg_time << " ms" << std::endl;
        }

        if (!fps_values.empty()) {
            double avg_fps = std::accumulate(fps_values.begin(), fps_values.end(), 0.0) / fps_values.size();
            std::cout << "Average FPS: " << avg_fps << std::endl;
        }

    } else {
        cv::Mat image = cv::imread(image_path);
        if (image.empty()) {
            std::cerr << "Could not open or find the image!" << std::endl;
            return -1;
        }

        std::cout << "Performing depth estimation on the image..." << std::endl;
        auto depth_start_time = get_current_time_fenced();
        cv::Mat depth_map = depth_estimator.infer(image);
        auto depth_end_time = get_current_time_fenced();

        cv::imshow("Depth Map", depth_map);
        cv::waitKey(0);

        std::string filename = image_path.substr(image_path.find_last_of("/\\") + 1);
        std::string model_type_str = modelTypeToString(model_type);
        filename = model_type_str + "_" + filename;
        std::string output_path = getContentPath(filename, "media/depth_estimation_results");
        cv::imwrite(output_path, depth_map);

        std::cout << "Depth map saved to " << output_path << std::endl;

        long long depth_time = to_ms(depth_end_time - depth_start_time);
        std::cout << "Depth estimation time: " << depth_time << " ms" << std::endl;
    }

    cv::destroyAllWindows();
    return 0;
}
