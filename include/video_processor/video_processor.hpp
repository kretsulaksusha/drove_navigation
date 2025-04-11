#ifndef DRONE_NAVIGATION_VIDEO_PROCESSOR_HPP
#define DRONE_NAVIGATION_VIDEO_PROCESSOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include "depth_estimation.hpp"
#include "kalman.hpp"
#include "feature_detector.hpp"
#include "time_meas.hpp"
#include "path_utils.hpp"

void selectROI(std::string &video_path);
void processVideo(std::string &video_path);
void mouseCallback(int event, int x, int y, int, void* userdata);

#endif //DRONE_NAVIGATION_VIDEO_PROCESSOR_HPP
