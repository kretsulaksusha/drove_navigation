#ifndef DRONE_NAVIGATION_DEPTH_ESTIMATION_HPP
#define DRONE_NAVIGATION_DEPTH_ESTIMATION_HPP

#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <numeric>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include "time_meas.hpp"
#include "path_utils.hpp"
#include "model_types.hpp"


/**
 * Extract contours from a frame.
 *
 * @param frame Input frame from the camera.
 * @return Contours of the input frame.
 */
cv::Mat contour_frame(const cv::Mat& frame);

/**
 * Test the depth estimation function.
 *
 * @param image_path Path to the input image.
 * @param enable_camera Enable camera input if true, otherwise use the image.
 * @param model_type Model type to use for depth estimation (DAV2, DAD, MiDaS).
 * @return 0 on success, non-zero on failure.
 */
int test_depth_estimation(std::string& image_path, bool enable_camera, ModelType model_type);

#endif //DRONE_NAVIGATION_DEPTH_ESTIMATION_HPP
