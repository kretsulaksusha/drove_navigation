#include "image_processor.hpp"


// FAST + BRIEF
cv::Ptr<cv::FastFeatureDetector> fast = cv::FastFeatureDetector::create();
cv::Ptr<cv::xfeatures2d::BriefDescriptorExtractor> brief = cv::xfeatures2d::BriefDescriptorExtractor::create();


LocalMap local_map;
DepthEstimator depth_estimator(ModelType::MiDaS);

void processImage(fs::path &image_path, ModelType model_type) {
    cv::Mat frame = cv::imread(image_path);
    cv::Size frame_size(frame.rows, frame.cols);
    if (frame.empty()) {
        std::cerr << "Error: Could not open or find the image." << std::endl;
        return;
    }

    // ------ Depth estimation ------
//    DepthEstimator depth_estimator(model_type);
    cv::Mat depth_map = depth_estimator.infer(frame);

    // Apply adaptive histogram equalization to enhance local contrast
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(4.0);  // Controls contrast amplification
    cv::Mat depth_enhanced;
    clahe->apply(depth_map, depth_enhanced);

    // Apply bilateral filtering to reduce noise while preserving edges
    cv::Mat depth_filtered;
    cv::bilateralFilter(depth_enhanced, depth_filtered, 9, 75, 75);

    // Display the processed depth maps for visualization
//    cv::imshow("Original Depth", depth_map);
//    cv::imshow("Filtered Depth", depth_filtered);

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    minMaxLoc(depth_filtered, &minVal, &maxVal, &minLoc, &maxLoc);

    // ------ Feature detection ------
    cv::Mat gray;
    cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    fast->detect(gray, keypoints);
    brief->compute(gray, keypoints, descriptors);

    // Apply NMS to filter out redundant keypoints
    applyNMS(keypoints);

    float median_depth = getMedianDepth(depth_filtered);

    // Filter keypoints based on a depth map
    std::vector<cv::KeyPoint> filtered_keypoints;
    for (auto& kp : keypoints) {
        int x = static_cast<int>(kp.pt.x);
        int y = static_cast<int>(kp.pt.y);

        if (x < 0 || x >= depth_filtered.cols || y < 0 || y >= depth_filtered.rows)
            continue;

        // Get depth value from depth map
        float depth_value = depth_filtered.at<float>(y, x);

        // Define a depth threshold range (example: 0.5m to 5m depth)
        if (depth_value >= median_depth) {
            filtered_keypoints.push_back(kp);
        }
    }

    std::vector<cv::Point2f> points;
    for (auto& kp : filtered_keypoints) points.push_back(kp.pt);

    std::vector<float> knn_distances = calculateKnnDistances(points);
    float eps = determineEps(knn_distances);
    int minPts = 4;   // Rule of thumb: Use 4 for 2D points

    auto clusters = clusterPoints(points, eps, minPts);

    // Update cluster centers and draw them
    std::vector<cv::Point2f> cluster_centers;

    for (auto & cluster : clusters) {
        cv::Point2f center(0, 0);
        for (auto& pt : cluster) center += pt;
        center *= (1.0f / static_cast<double>(cluster.size()));

        // Store the current cluster center
        cluster_centers.push_back(center);

        // Draw each point in the cluster
        for (auto& pt : cluster) {
            circle(frame, pt, 2, cv::Scalar(255, 0, 0), -1);
        }

        // Draw the center of the cluster
        circle(frame, center, 6, cv::Scalar(0, 255, 0), 2);
    }

    local_map.update(clusters, depth_filtered, frame_size);

    std::cout << local_map.getObstacles().size() << " obstacles detected." << std::endl;

    // Draw current clusters with distance info
    for (const auto& cluster : clusters) {
        if (cluster.empty()) continue;

        cv::Point2f center(0, 0);
        for (const auto& pt : cluster) center += pt;
        center *= (1.0f / cluster.size());

        // Calculate average depth for the cluster
        float depth_sum = 0;
        int depth_count = 0;
        for (const auto& pt : cluster) {
            int x = static_cast<int>(pt.x);
            int y = static_cast<int>(pt.y);
            if (x >= 0 && x < depth_filtered.cols && y >= 0 && y < depth_filtered.rows) {
                depth_sum += depth_filtered.at<float>(y, x);
                depth_count++;
            }
        }
        float avg_depth = depth_count > 0 ? depth_sum / depth_count : 0;

        if (avg_depth < 200.0) {
            std::cout << "Distance to cluster center: " << avg_depth << " m" << std::endl;
        }
    }
}
