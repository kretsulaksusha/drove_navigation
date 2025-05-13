#include "depth_estimator.hpp"

namespace fs = std::filesystem;

/**
 * @brief Constructor for DepthEstimator.
 *
 * @param model_type The type of model to use (MiDaS, DAV2, DAD).
 * @param thread_count Number of threads for ONNX Runtime.
 */
DepthEstimator::DepthEstimator(ModelType model_type, int thread_count)
        : model_type_(model_type),
          env_(ORT_LOGGING_LEVEL_WARNING, "DepthEstimator"),
          session_options_(),
          memory_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {
    session_options_.SetIntraOpNumThreads(thread_count);
    session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    fs::path cwd = fs::current_path();
    std::string base_path = (cwd.filename() == "drove_navigation") ? "./models/" : "../models/";

    if (model_type == ModelType::MiDaS) {
        std::string model_path = base_path + "model-small.onnx";
        try {
            midas_net_ = cv::dnn::readNetFromONNX(model_path);
        } catch (const cv::Exception& e) {
            throw std::runtime_error("Failed to load MiDaS model: " + std::string(e.what()));
        }
    } else {
        std::string model_file = (model_type == ModelType::DAV2)
                                 ? "depth_anything_v2_vits.onnx"
                                 : "distill-any-depth-small-hf.onnx";
        std::string model_path = base_path + model_file;
        session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(), session_options_);

        Ort::AllocatorWithDefaultOptions allocator;
        input_name_ = session_->GetInputNameAllocated(0, allocator).get();
        output_name_ = session_->GetOutputNameAllocated(0, allocator).get();
    }
}

/**
 * @brief Infer depth from the given frame.
 *
 * @param frame The input image frame.
 * @return cv::Mat The depth map.
 */
cv::Mat DepthEstimator::infer(const cv::Mat& frame) {
    switch (model_type_) {
        case ModelType::MiDaS:
            return inferMiDaS(frame);
        case ModelType::DAV2:
        case ModelType::DAD:
            return inferDAV2orDAD(frame);
        default:
            throw std::runtime_error("Unknown model type.");
    }
}

/**
 * @brief Infer depth using MiDaS model.
 *
 * @param frame The input image frame.
 * @return cv::Mat The depth map.
 */
cv::Mat DepthEstimator::inferMiDaS(const cv::Mat& frame) {
    cv::Mat resized, blob;
    cv::resize(frame, resized, cv::Size(256, 256));
    blob = cv::dnn::blobFromImage(resized, 1.0 / 255.0, cv::Size(256, 256), cv::Scalar(0, 0, 0), true, false);

    midas_net_.setInput(blob);
    cv::Mat output = midas_net_.forward();

    auto* output_data = reinterpret_cast<float*>(output.data);
    cv::Mat result(output.size[1], output.size[2], CV_32F, output_data);

    cv::Mat normalized, final_depth;
    cv::normalize(result, normalized, 0, 1, cv::NORM_MINMAX);
    cv::resize(normalized, final_depth, frame.size());
    final_depth.convertTo(final_depth, CV_8UC1, 255);

    return final_depth;
}

/**
 * @brief Infer depth using DAV2 or DAD model.
 *
 * @param frame The input image frame.
 * @return cv::Mat The depth map.
 */
cv::Mat DepthEstimator::inferDAV2orDAD(const cv::Mat& frame) {
    cv::Size input_size(518, 518);
    cv::Mat resized, float_img;
    cv::resize(frame, resized, input_size);
    resized.convertTo(float_img, CV_32F, 1.0 / 255.0);
    float_img = (float_img - 0.5f) / 0.5f;

    // Convert HWC to CHW
    std::vector<float> input_tensor_values;
    input_tensor_values.reserve(3 * 518 * 518);
    for (int c = 0; c < 3; ++c) {
        for (int y = 0; y < float_img.rows; ++y) {
            for (int x = 0; x < float_img.cols; ++x) {
                input_tensor_values.push_back(float_img.at<cv::Vec3f>(y, x)[c]);
            }
        }
    }

    // Define input shape {batch_size, channels, height, width}
    std::vector<int64_t> input_shape = {1, 3, 518, 518};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info_, input_tensor_values.data(), input_tensor_values.size(),
            input_shape.data(), input_shape.size()
    );

    // Run inference
    const char* input_names[] = {input_name_.c_str()};
    const char* output_names[] = {output_name_.c_str()};

    auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names, &input_tensor, 1,
            output_names, 1
    );

    // Get output
    auto* output_data = output_tensors.front().GetTensorMutableData<float>();

    // Postprocess: output shape usually {1, 1, H, W}
    cv::Mat depth(518, 518, CV_32F, output_data);

    // Resize back to the original size
    cv::Mat resized_depth, depth_map;
    cv::resize(depth, resized_depth, frame.size(), 0, 0, cv::INTER_CUBIC);

    // Normalize depth map for visualization
    cv::normalize(resized_depth, depth_map, 0, 255, cv::NORM_MINMAX);
    depth_map.convertTo(depth_map, CV_8UC1);

    return depth_map;
}
