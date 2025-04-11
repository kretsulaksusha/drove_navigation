#include "video_processor.hpp"

// Configuration defines
#define USE_EKF 1                    // 0=Kalman,               1=EKF
#define MEASURE_TIME 1               // 0=No timing,            1=Measure timing
#define SELECT_ROI 0                 // 0=Use full frame,       1=Select ROI
#define SHOW_PREDICTED_POSITION 0    // 0=No predicted cluster, 1=Show predicted cluster

#if USE_EKF
typedef ExtendedKalmanFilter Filter;
#else
typedef KalmanFilter Filter;
#endif

// FAST + BRIEF
cv::Ptr<cv::FastFeatureDetector> fast = cv::FastFeatureDetector::create();
cv::Ptr<cv::xfeatures2d::BriefDescriptorExtractor> brief = cv::xfeatures2d::BriefDescriptorExtractor::create();

// ROI coordinates
int x_min = 36000, y_min = 36000, x_max = 0, y_max = 0;


void processVideo(std::string &video_path) {
    cv::VideoCapture video(video_path);
    if (!video.isOpened()) {
        std::cerr << "Error: Could not open video." << std::endl;
        return;
    }

    // Get video properties for the output video
    int frame_width = static_cast<int>(video.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(video.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = video.get(cv::CAP_PROP_FPS);

    std::string output_video_path = getContentPath("output_", "media/video_results");

    // Create VideoWriter object
    cv::VideoWriter output_video(
            output_video_path +
#if USE_EKF
            "EKF.avi",
#else
            "KF.avi",
#endif
            cv::VideoWriter::fourcc('M','J','P','G'), // Codec
            fps, // Frames per second
            cv::Size(frame_width, frame_height)
    );

    if (!output_video.isOpened()) {
        std::cerr << "Error: Could not create output video file." << std::endl;
        return;
    }

    // Create VideoWriter for filtered depth output
//    cv::VideoWriter depth_grayscale_writer(
//            "filtered_depth_output.avi",
//            cv::VideoWriter::fourcc('M','J','P','G'), // or use 'XVID'
//            fps,
//            cv::Size(frame_width, frame_height),
//            false // grayscale
//    );
//
//    if (!depth_grayscale_writer.isOpened()) {
//        std::cerr << "Error: Could not create video writer." << std::endl;
//        return;
//    }

    std::unordered_map<int, Filter> trackers;
    std::vector<long long> filter_times;
    cv::Mat frame;
    int frame_count = 0;

    while (video.read(frame)) {
#if MEASURE_TIME
        auto start_time = get_current_time_fenced();
#endif
        // ------ Depth estimation ------
        cv::Mat depth_map = depth_estimation(frame);

        // Convert to grayscale
        cv::Mat depth_map_gray;
        cv::cvtColor(depth_map, depth_map_gray, cv::COLOR_BGR2GRAY);

        // Apply adaptive histogram equalization to enhance local contrast
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
        clahe->setClipLimit(4.0);  // Controls contrast amplification
        cv::Mat depth_enhanced;
        clahe->apply(depth_map_gray, depth_enhanced);

        // Apply bilateral filtering to reduce noise while preserving edges
        cv::Mat depth_filtered;
        cv::bilateralFilter(depth_enhanced, depth_filtered, 9, 75, 75);

//        depth_grayscale_writer.write(depth_filtered);

#if !MEASURE_TIME
        cv::imshow("Original Depth", depth_map);
        cv::imshow("Filtered Depth", depth_filtered);
#endif

        if (depth_filtered.type() != CV_32F) {
            depth_filtered.convertTo(depth_filtered, CV_32F);
        }

        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        minMaxLoc(depth_filtered, &minVal, &maxVal, &minLoc, &maxLoc);
//        std::cout << "min val: " << minVal << std::endl;
//        std::cout << "max val: " << maxVal << std::endl;

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

        // Filter keypoints based on depth map
        std::vector<cv::KeyPoint> filtered_keypoints;
        for (auto& kp : keypoints) {
            int x = static_cast<int>(kp.pt.x);
            int y = static_cast<int>(kp.pt.y);

            if (x < 0 || x >= depth_filtered.cols || y < 0 || y >= depth_filtered.rows)
                continue;

            // Get depth value from depth map
            float depth_value = depth_filtered.at<float>(y, x);
//            std::cout << "`depth_value` at " << x << " and " << y << ": " << depth_value << std::endl;

            // Define a depth threshold range (example: 0.5m to 5m depth)
            if (depth_value >= median_depth) {
                filtered_keypoints.push_back(kp);
            }
        }

        std::vector<cv::Point2f> points;
        for (auto& kp : filtered_keypoints) points.push_back(kp.pt);
//        for (auto& kp : keypoints) points.push_back(kp.pt);

        std::vector<float> knn_distances = calculateKnnDistances(points);
        float eps = determineEps(knn_distances);
        int minPts = 4;   // Rule of thumb: Use 4 for 2D points

        auto clusters = clusterPoints(points, eps, minPts);

        for (int i = 0; i < clusters.size(); ++i) {
            cv::Point2f center(0, 0);
            for (auto& pt : clusters[i]) center += pt;
            center *= (1.0f / static_cast<double>(clusters[i].size()));

            if (trackers.find(i) == trackers.end()) {
                trackers[i] = decltype(trackers)::mapped_type(center.x, center.y);
            }

            trackers[i].predict(1.0f / 30);
            trackers[i].update(center.x, center.y);

            for (auto& pt : clusters[i]) {
                circle(frame, pt, 2, cv::Scalar(255, 0, 0), -1);
            }

            circle(frame, center, 6, cv::Scalar(0, 255, 0), 2);
#if SHOW_PREDICTED_POSITION
            auto predicted = trackers[i].getPredictedPosition();
            circle(frame, predicted, 6, cv::Scalar(0, 0, 255), 2);
            line(frame, center, predicted, cv::Scalar(0, 255, 255), 2);
#endif
        }

        // Write the frame to the output video
        output_video.write(frame);

#if MEASURE_TIME
        auto end_time = get_current_time_fenced();
        long long frame_time = to_mcs(end_time - start_time);
        std::string time_text = "Frame time: " + std::to_string(frame_time) + " mcs";
        cv::putText(frame, time_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

        frame_count++;
#endif

        imshow("Tracking", frame);
        if (cv::waitKey(30) == 27) break;
    }

    video.release();
    cv::destroyAllWindows();
}

void mouseCallback(int event, int x, int y, int, void* userdata) {
    auto* framePtr = static_cast<cv::Mat*>(userdata);

    if (event == cv::EVENT_LBUTTONDOWN) {
        x_min = cv::min(x, x_min);
        y_min = cv::min(y, y_min);
        x_max = cv::max(x, x_max);
        y_max = cv::max(y, y_max);

        rectangle(*framePtr, cv::Point(x_min, y_min), cv::Point(x_max, y_max), cv::Scalar(255, 255, 0), 2);
        imshow("Video Player", *framePtr);
    }
}

void selectROI(std::string &video_path) {
    cv::VideoCapture video(video_path);
    if (!video.isOpened()) {
        std::cerr << "Error: Could not open video." << std::endl;
        return;
    }

    cv::Mat frame;
    video.read(frame);

#if SELECT_ROI
    cv::namedWindow("Video Player");
    setMouseCallback("Video Player", mouseCallback, &frame);
    imshow("Video Player", frame);
    cv::waitKey(0);

    cv::Rect roi(x_min, y_min, x_max - x_min, y_max - y_min);
    cv::Mat image_roi = frame(roi);
    imshow("Selected ROI", image_roi);
    cv::waitKey(500);
#else
    // When not selecting ROI, use the entire frame
    cv::Rect roi(0, 0, frame.cols, frame.rows);
    cv::Mat image_roi = frame(roi);
#endif

    cv::Mat gray_roi;
    cvtColor(image_roi, gray_roi, cv::COLOR_BGR2GRAY);
    std::vector<cv::KeyPoint> roiKeypoints;
    cv::Mat roiDescriptors;
    fast->detect(gray_roi, roiKeypoints);
    brief->compute(gray_roi, roiKeypoints, roiDescriptors);

    processVideo(video_path);
}
