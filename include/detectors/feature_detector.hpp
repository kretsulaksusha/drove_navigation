#ifndef DRONE_NAVIGATION_FEATURE_DETECTOR_HPP
#define DRONE_NAVIGATION_FEATURE_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <vector>
#include <algorithm>
#include "path_utils.hpp"

/**
 * Apply Non-Maximum Suppression (NMS) to filter out redundant keypoints.
 *
 * @param keypoints Keypoints to be filtered.
 * @param overlap_threshold Overlap threshold for NMS.
 */
void applyNMS(std::vector<cv::KeyPoint>& keypoints, float overlap_threshold = 0.05f);

/**
 * Cluster points using a simple DBSCAN-like algorithm.
 *
 * @param points Points to be clustered.
 * @param eps Distance threshold for clustering.
 * @param minPts Minimum number of points to form a cluster.
 * @return A vector of clusters, where each cluster is a vector of points.
 */
std::vector<std::vector<cv::Point2f>> clusterPoints(const std::vector<cv::Point2f>& points, float eps, int minPts);

/**
 * Calculate k-th nearest neighbor distance (used to estimate `eps`).
 *
 * @param points Points to be clustered.
 * @param k Number of nearest neighbors to consider.
 * @return A vector of distances to the k-th nearest neighbor for each point.
 */
std::vector<float> calculateKnnDistances(const std::vector<cv::Point2f>& points, int k = 4);

/**
 * Determine the optimal `eps` value based on K-NN distances.
 *
 * @param knn_distances Distances to the k-th nearest neighbor for each point.
 * @return The optimal `eps` value.
 */
float determineEps(const std::vector<float>& knn_distances);

/**
 * Getting the median depth from the depth map.
 *
 * @param depth_map Depth map to be processed.
 * @return The median depth value.
 */
float getMedianDepth(const cv::Mat& depth_map);

/**
 * Test the FAST feature detector.
 *
 * @return 0 on success, non-zero on failure.
 */
int test_fast_detector(std::string &image_path);

#endif //DRONE_NAVIGATION_FEATURE_DETECTOR_HPP
