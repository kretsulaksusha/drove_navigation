#ifndef DRONE_NAVIGATION_KALMAN_HPP
#define DRONE_NAVIGATION_KALMAN_HPP

#include <iostream>
#include <random>
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>

// Standard Kalman Filter declaration
struct KalmanFilter {
    Eigen::Vector4f state; // [x, y, vx, vy]
    Eigen::Matrix4f P;     // State covariance matrix
    Eigen::Matrix4f Q;     // Process noise covariance
    Eigen::Matrix<float, 2, 4> H; // Measurement matrix
    Eigen::Matrix2f R;     // Measurement noise covariance

    // Constructors
    KalmanFilter();
    KalmanFilter(float x, float y);

    // Methods
    void predict(float dt);
    void update(float x, float y);
    [[nodiscard]] cv::Point2f getPredictedPosition() const;
};

// Extended Kalman Filter declaration
struct ExtendedKalmanFilter {
    Eigen::Vector4f state; // [x, y, vx, vy]
    Eigen::Matrix4f P;     // State covariance matrix
    Eigen::Matrix4f Q;     // Process noise covariance
    Eigen::Matrix<float, 2, 4> H; // Measurement matrix
    Eigen::Matrix2f R;     // Measurement noise covariance

    // Constructors
    ExtendedKalmanFilter();
    ExtendedKalmanFilter(float x, float y);

    // Methods
    void predict(float dt);
    void update(float x, float y);
    [[nodiscard]] cv::Point2f getPredictedPosition() const;
};

void testKalmanFilter();


#endif //DRONE_NAVIGATION_KALMAN_HPP
