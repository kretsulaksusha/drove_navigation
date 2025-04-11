#include "feature_detector.hpp"


void applyNMS(std::vector<cv::KeyPoint>& keypoints, float overlap_threshold) {
    std::vector<bool> to_remove(keypoints.size(), false);

    for (size_t i = 0; i < keypoints.size(); ++i) {
        if (to_remove[i]) continue;
        for (size_t j = i + 1; j < keypoints.size(); ++j) {
            if (to_remove[j]) continue;

            float overlap = cv::KeyPoint::overlap(keypoints[i], keypoints[j]);
            if (overlap > overlap_threshold) {
                // Mark the keypoint with the lower response for removal
                if (keypoints[i].response < keypoints[j].response) {
                    to_remove[i] = true;
                } else {
                    to_remove[j] = true;
                }
            }
        }
    }

    // Remove the marked keypoints
    keypoints.erase(std::remove_if(keypoints.begin(), keypoints.end(),
                                   [&](cv::KeyPoint& kp) { return to_remove[&kp - &keypoints[0]]; }), keypoints.end());
}

std::vector<std::vector<cv::Point2f>> clusterPoints(const std::vector<cv::Point2f>& points, float eps, int minPts) {
    std::vector<bool> visited(points.size(), false);
    std::vector<std::vector<cv::Point2f>> clusters;

    auto regionQuery = [&](int idx) {
        std::vector<int> neighbors;
        for (size_t i = 0; i < points.size(); ++i) {
            if (norm(points[idx] - points[i]) <= eps)
                neighbors.push_back(i);
        }
        return neighbors;
    };

    for (size_t i = 0; i < points.size(); ++i) {
        if (visited[i]) continue;
        visited[i] = true;
        auto neighbors = regionQuery(i);

        if (neighbors.size() < minPts) continue;

        std::vector<cv::Point2f> cluster;
        cluster.push_back(points[i]);

        for (size_t j = 0; j < neighbors.size(); ++j) {
            int idx = neighbors[j];
            if (!visited[idx]) {
                visited[idx] = true;
                auto new_neighbors = regionQuery(idx);
                if (new_neighbors.size() >= minPts) {
                    neighbors.insert(neighbors.end(), new_neighbors.begin(), new_neighbors.end());
                }
            }
            cluster.push_back(points[idx]);
        }
        clusters.push_back(cluster);
    }

    return clusters;
}

std::vector<float> calculateKnnDistances(const std::vector<cv::Point2f>& points, int k) {
    std::vector<float> knn_distances;
    for (size_t i = 0; i < points.size(); ++i) {
        std::vector<float> distances;
        for (size_t j = 0; j < points.size(); ++j) {
            if (i == j) continue;
            distances.push_back(cv::norm(points[i] - points[j]));
        }
        std::sort(distances.begin(), distances.end());
        knn_distances.push_back(distances[k-1]);  // Distance to k-th nearest neighbor
    }

    std::sort(knn_distances.begin(), knn_distances.end());
    return knn_distances;
}

float determineEps(const std::vector<float>& knn_distances) {
    int elbow_index = static_cast<int>(knn_distances.size() * 0.9);  // Approx. 90% quantile
    return knn_distances[elbow_index];
}

float getMedianDepth(const cv::Mat& depth_map) {
    // Flatten the depth map to a single vector of depth values
    std::vector<float> depth_values;

    for (int y = 0; y < depth_map.rows; ++y) {
        for (int x = 0; x < depth_map.cols; ++x) {
            float depth_value = depth_map.at<float>(y, x);
            if (depth_value >= 0.5f && depth_value <= 5.0f) {  // Depth range condition
                depth_values.push_back(depth_value);
            }
        }
    }

    std::sort(depth_values.begin(), depth_values.end());

    size_t size = depth_values.size();
    if (size == 0) return 0.0f;
    if (size % 2 == 0) {
        // Even number of elements -> take the average of the two middle values
        return (depth_values[size / 2 - 1] + depth_values[size / 2]) / 2.0f;
    } else {
        // Odd number of elements -> take the middle value
        return depth_values[size / 2];
    }
}

int test_fast_detector(std::string &image_path) {
    cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);

    if (image.empty()) {
        std::cerr << "Failed to load image!" << std::endl;
        return -1;
    }

    // Detect features using FAST detector
    std::vector<cv::KeyPoint> keypoints;
    cv::Ptr<cv::FastFeatureDetector> detector = cv::FastFeatureDetector::create(25); // threshold
    detector->detect(image, keypoints);

    std::cout << "Original keypoints: " << keypoints.size() << std::endl;

    // Apply Non-Maximum Suppression
    applyNMS(keypoints, 0.2f); // overlap threshold
    std::cout << "Keypoints after NMS: " << keypoints.size() << std::endl;

    // Convert keypoints to Point2f for clustering
    std::vector<cv::Point2f> points;
    points.reserve(keypoints.size());
    for (const auto& kp : keypoints) {
        points.push_back(kp.pt);
    }

    // Calculate kNN distances and determine eps
    auto knn_distances = calculateKnnDistances(points, 3); // k = 3
    float eps = determineEps(knn_distances);
    std::cout << "Determined eps for clustering: " << eps << std::endl;

    // Perform clustering
    auto clusters = clusterPoints(points, eps, 3); // minPts = 3
    std::cout << "Number of clusters: " << clusters.size() << std::endl;

    // Draw results
    cv::Mat output_image;
    cv::cvtColor(image, output_image, cv::COLOR_GRAY2BGR);

    // Assign random colors to clusters
    std::vector<cv::Scalar> colors;
    cv::RNG rng(12345);
    colors.reserve(clusters.size());
    for (size_t i = 0; i < clusters.size(); ++i) {
        colors.emplace_back(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
    }

    for (size_t i = 0; i < clusters.size(); ++i) {
        for (const auto& pt : clusters[i]) {
            cv::circle(output_image, pt, 3, colors[i], -1);
        }
    }

    cv::imshow("Clustered Features", output_image);
    cv::waitKey(0);

    std::string filename = image_path.substr(image_path.find_last_of("/\\") + 1);
    std::string output_path = getContentPath(filename, "media/feature_detection_results");
    cv::imwrite(output_path, output_image);

    std::cout << "Feature detection saved to " << output_path << std::endl;

    return 0;
}
