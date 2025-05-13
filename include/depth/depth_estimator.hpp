#ifndef DRONE_NAVIGATION_DEPTH_ESTIMATOR_HPP
#define DRONE_NAVIGATION_DEPTH_ESTIMATOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <filesystem>
#include <string>
#include "model_types.hpp"


/**
 * @brief Estimating depth from images using different models.
 */
class DepthEstimator {
public:
    explicit DepthEstimator(ModelType model_type, int thread_count = 8);
    cv::Mat infer(const cv::Mat& frame);

private:
    ModelType model_type_;

    // ONNX Runtime (for DAV2, DAD)
    Ort::Env env_;
    Ort::SessionOptions session_options_;
    std::unique_ptr<Ort::Session> session_; // Use a pointer for optional init
    Ort::MemoryInfo memory_info_;
    std::string input_name_;
    std::string output_name_;

    // OpenCV DNN (for MiDaS)
    cv::dnn::Net midas_net_;

    // Internal inference methods
    cv::Mat inferDAV2orDAD(const cv::Mat& frame);
    cv::Mat inferMiDaS(const cv::Mat& frame);
};

#endif // DRONE_NAVIGATION_DEPTH_ESTIMATOR_HPP
