#ifndef DRONE_NAVIGATION_DEPTH_ESTIMATION_HPP
#define DRONE_NAVIGATION_DEPTH_ESTIMATION_HPP

#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <numeric>
#include "time_meas.hpp"
#include "path_utils.hpp"

/**
 * Perform monocular depth estimation.
 *
 * @param frame Input frame from the camera.
 * @return Depth map of the input frame.
 */
cv::Mat depth_estimation(cv::Mat frame);

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
 * @return 0 on success, non-zero on failure.
 */
int test_depth_estimation(std::string &image_path, bool enable_camera);

#endif //DRONE_NAVIGATION_DEPTH_ESTIMATION_HPP
