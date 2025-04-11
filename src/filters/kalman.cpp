#include "kalman.hpp"

// ------- Kalman Filter Implementation -------

KalmanFilter::KalmanFilter() : KalmanFilter(0, 0) {}

KalmanFilter::KalmanFilter(float x, float y) {
    state << x, y, 0, 0;
    P = Eigen::Matrix4f::Identity() * 1000;
    Q = Eigen::Matrix4f::Identity() * 0.1f;
    H << 1, 0, 0, 0,
         0, 1, 0, 0;
    R = Eigen::Matrix2f::Identity() * 10;
}

void KalmanFilter::predict(float dt) {
    // Linear prediction (constant velocity model)
    Eigen::Matrix4f F;
    F << 1, 0, dt, 0,
         0, 1, 0,  dt,
         0, 0, 1,  0,
         0, 0, 0,  1;
    state = F * state;
    P = F * P * F.transpose() + Q;
}

void KalmanFilter::update(float x, float y) {
    Eigen::Vector2f z(x, y);
    Eigen::Vector2f y_vec = z - H * state;
    Eigen::Matrix2f S = H * P * H.transpose() + R;
    Eigen::Matrix<float, 4, 2> K = P * H.transpose() * S.inverse();
    state += K * y_vec;
    P = (Eigen::Matrix4f::Identity() - K * H) * P;
}

cv::Point2f KalmanFilter::getPredictedPosition() const {
    return {state(0), state(1)};
}

// ------- Extended Kalman Filter (EKF) Implementation -------

ExtendedKalmanFilter::ExtendedKalmanFilter() : ExtendedKalmanFilter(0, 0) {}

ExtendedKalmanFilter::ExtendedKalmanFilter(float x, float y) {
    state << x, y, 0, 0;
    P = Eigen::Matrix4f::Identity() * 1000;
    Q = Eigen::Matrix4f::Identity() * 0.1f;
    H << 1, 0, 0, 0,
         0, 1, 0, 0;
    R = Eigen::Matrix2f::Identity() * 10;
}

void ExtendedKalmanFilter::predict(float dt) {
    // Nonlinear prediction (could add more complex models here)
    float vx = state[2], vy = state[3];
    state[0] += vx * dt + 0.5f * dt * dt; // with simple acceleration
    state[1] += vy * dt + 0.5f * dt * dt;

    Eigen::Matrix4f F;
    F << 1, 0, dt, 0.5f*dt*dt,
         0, 1, 0,  dt,
         0, 0, 1,  0,
         0, 0, 0,  1;
    P = F * P * F.transpose() + Q;
}

void ExtendedKalmanFilter::update(float x, float y) {
    Eigen::Vector2f z(x, y);
    Eigen::Vector2f y_vec = z - H * state;
    Eigen::Matrix2f S = H * P * H.transpose() + R;
    Eigen::Matrix<float, 4, 2> K = P * H.transpose() * S.inverse();
    state += K * y_vec;
    P = (Eigen::Matrix4f::Identity() - K * H) * P;
}

cv::Point2f ExtendedKalmanFilter::getPredictedPosition() const {
    return {state(0), state(1)};
}

void testKalmanFilter() {
    KalmanFilter kf(0, 0);

    float true_x = 0;
    float true_y = 0;
    float velocity_x = 1.0f;
    float velocity_y = 0.5f;

    std::default_random_engine generator;
    std::normal_distribution<float> noise(0.0f, 1.0f);

    const int num_steps = 70;
    const float dt = 1.0f;

    cv::Mat display = cv::Mat::zeros(900, 1500, CV_8UC3);

    for (int step = 0; step < num_steps; ++step) {
        true_x += velocity_x * dt;
        true_y += velocity_y * dt;

        float measured_x = true_x + noise(generator);
        float measured_y = true_y + noise(generator);

        kf.predict(dt);
        kf.update(measured_x, measured_y);

        cv::Point2f predicted = kf.getPredictedPosition();

        std::cout << "Step: " << step
                  << " | Measured: (" << measured_x << ", " << measured_y << ")"
                  << " | Predicted: (" << predicted.x << ", " << predicted.y << ")"
                  << " | True: (" << true_x << ", " << true_y << ")\n";

        cv::circle(display, cv::Point(measured_x * 20 + 50, measured_y * 20 + 50), 3,
                   cv::Scalar(0, 0, 255), -1); // Measured (red)
        cv::circle(display, cv::Point(predicted.x * 20 + 50, predicted.y * 20 + 50), 3,
                   cv::Scalar(255, 0, 0), -1); // Predicted (blue)
        cv::circle(display, cv::Point(true_x * 20 + 50, true_y * 20 + 50), 3,
                   cv::Scalar(0, 255, 0), -1); // True (green)

        cv::imshow("Kalman Filter Test (press any key to increase the step)", display);
        cv::waitKey(0);
    }

    cv::waitKey(0);
}
