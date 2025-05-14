#ifndef DRONE_NAVIGATION_IMAGE_PROCESSOR_HPP
#define DRONE_NAVIGATION_IMAGE_PROCESSOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include "depth_estimator.hpp"
#include "kalman.hpp"
#include "feature_detector.hpp"
#include "time_meas.hpp"
#include "path_utils.hpp"
#include "model_types.hpp"

namespace fs = std::filesystem;

#define MEASURE_TIME 1               // Timing measurement
#define MAP_HISTORY 10               // Frames to keep in history
#define CLUSTER_MATCH_DISTANCE 0.5f  // Meters to consider same obstacle
#define MAP_SCALE 50.0f              // Pixels per meter
#define MAP_WIDTH 2500                // Map width in pixels
#define MAP_HEIGHT 2500               // Map height in pixels
#define CAMERA_FOV_X 60.0f           // Horizontal FOV in degrees
#define CAMERA_FOV_Y 45.0f           // Vertical FOV in degrees

struct Obstacle {
    cv::Point2f image_position;      // Position in image coordinates
    cv::Point2f world_position;      // Position in world coordinates (meters)
    cv::Point2f velocity;            // Velocity in world coordinates (m/s)
    float average_depth;             // Depth in meters
    int life_time;                   // Frames tracked
    int last_seen;                   // Frame last seen
    std::deque<cv::Point2f> position_history;
};

class LocalMap {
private:
    std::unordered_map<int, Obstacle> obstacles;
    int current_frame = 0;
    int next_id = 0;
    cv::Mat map_image;
    float time_per_frame = 0.033f;   // Assuming 30fps

public:
    LocalMap() {
        map_image = cv::Mat::zeros(MAP_HEIGHT, MAP_WIDTH, CV_8UC3);
        // Draw grid lines
        for (int x = 0; x <= MAP_WIDTH; x += MAP_SCALE) {
            cv::line(map_image, cv::Point(x, 0), cv::Point(x, MAP_HEIGHT), cv::Scalar(50, 50, 50), 1);
        }
        for (int y = 0; y <= MAP_HEIGHT; y += MAP_SCALE) {
            cv::line(map_image, cv::Point(0, y), cv::Point(MAP_WIDTH, y), cv::Scalar(50, 50, 50), 1);
        }
        // Draw axes
        cv::line(map_image, cv::Point(MAP_WIDTH/2, MAP_HEIGHT),
                 cv::Point(MAP_WIDTH/2, 0), cv::Scalar(200, 200, 200), 2);
        cv::line(map_image, cv::Point(MAP_WIDTH/2, MAP_HEIGHT),
                 cv::Point(0, MAP_HEIGHT*0.75), cv::Scalar(200, 200, 200), 2);
        cv::line(map_image, cv::Point(MAP_WIDTH/2, MAP_HEIGHT),
                 cv::Point(MAP_WIDTH, MAP_HEIGHT*0.75), cv::Scalar(200, 200, 200), 2);
    }

    // Convert image coordinates to world coordinates (meters)
    cv::Point2f imageToWorld(cv::Point2f img_pt, float depth, int img_width, int img_height) {
        // Normalize image coordinates to [-1, 1] range
        float x_normalized = (2.0f * img_pt.x / img_width) - 1.0f;
        float y_normalized = (2.0f * img_pt.y / img_height) - 1.0f;

        // Convert to angles using camera FOV
        float x_angle = x_normalized * (CAMERA_FOV_X * M_PI / 360.0f);
        float y_angle = y_normalized * (CAMERA_FOV_Y * M_PI / 360.0f);

        // Calculate world coordinates
        float world_x = depth * tan(x_angle);
        float world_y = depth / cos(x_angle);  // More accurate projection

        return cv::Point2f(world_x, world_y);
    }

    // Convert world coordinates to map coordinates (pixels)
    cv::Point worldToMap(cv::Point2f world_pt) {
        int map_x = MAP_WIDTH/2 + static_cast<int>(world_pt.x * MAP_SCALE);
        int map_y = MAP_HEIGHT - static_cast<int>(world_pt.y * MAP_SCALE);
        return cv::Point(map_x, map_y);
    }

    void update(const std::vector<std::vector<cv::Point2f>>& clusters,
                const cv::Mat& depth_map,
                const cv::Size& image_size) {
        current_frame++;

        // Fade out old map contents
        map_image = map_image * 0.7;
        // Redraw grid and axes
        for (int x = 0; x <= MAP_WIDTH; x += MAP_SCALE) {
            cv::line(map_image, cv::Point(x, 0), cv::Point(x, MAP_HEIGHT), cv::Scalar(50, 50, 50), 1);
        }
        for (int y = 0; y <= MAP_HEIGHT; y += MAP_SCALE) {
            cv::line(map_image, cv::Point(0, y), cv::Point(MAP_WIDTH, y), cv::Scalar(50, 50, 50), 1);
        }
        cv::line(map_image, cv::Point(MAP_WIDTH/2, MAP_HEIGHT),
                 cv::Point(MAP_WIDTH/2, 0), cv::Scalar(200, 200, 200), 2);
        cv::line(map_image, cv::Point(MAP_WIDTH/2, MAP_HEIGHT),
                 cv::Point(0, MAP_HEIGHT*0.75), cv::Scalar(200, 200, 200), 2);
        cv::line(map_image, cv::Point(MAP_WIDTH/2, MAP_HEIGHT),
                 cv::Point(MAP_WIDTH, MAP_HEIGHT*0.75), cv::Scalar(200, 200, 200), 2);

        // Decrease life of all obstacles
        for (auto& pair : obstacles) {
            if (pair.second.last_seen < current_frame - 1) {
                pair.second.life_time--;
            }
        }

        // Match new clusters to existing obstacles
        std::vector<bool> matched_clusters(clusters.size(), false);

        for (auto& pair : obstacles) {
            Obstacle& obs = pair.second;
            float min_dist = FLT_MAX;
            int best_cluster = -1;

            for (size_t i = 0; i < clusters.size(); ++i) {
                if (matched_clusters[i]) continue;

                // Compute cluster center in image coordinates
                cv::Point2f img_center(0, 0);
                for (const auto& pt : clusters[i]) img_center += pt;
                img_center *= (1.0f / clusters[i].size());

                // Calculate average depth for the cluster
                float depth_sum = 0;
                int depth_count = 0;
                for (const auto& pt : clusters[i]) {
                    int x = static_cast<int>(pt.x);
                    int y = static_cast<int>(pt.y);
                    if (x >= 0 && x < depth_map.cols && y >= 0 && y < depth_map.rows) {
                        depth_sum += depth_map.at<float>(y, x);
                        depth_count++;
                    }
                }
                float avg_depth = depth_count > 0 ? depth_sum / depth_count : 0;

                // Convert to world coordinates
                cv::Point2f world_pos = imageToWorld(img_center, avg_depth, image_size.width, image_size.height);

                // Calculate distance in world coordinates
                float dist = cv::norm(world_pos - obs.world_position);
                if (dist < CLUSTER_MATCH_DISTANCE && dist < min_dist) {
                    min_dist = dist;
                    best_cluster = i;
                }
            }

            if (best_cluster != -1) {
                matched_clusters[best_cluster] = true;

                // Compute new cluster center
                cv::Point2f img_center(0, 0);
                for (const auto& pt : clusters[best_cluster]) img_center += pt;
                img_center *= (1.0f / clusters[best_cluster].size());

                // Calculate average depth
                float depth_sum = 0;
                int depth_count = 0;
                for (const auto& pt : clusters[best_cluster]) {
                    int x = static_cast<int>(pt.x);
                    int y = static_cast<int>(pt.y);
                    if (x >= 0 && x < depth_map.cols && y >= 0 && y < depth_map.rows) {
                        depth_sum += depth_map.at<float>(y, x);
                        depth_count++;
                    }
                }
                float avg_depth = depth_count > 0 ? depth_sum / depth_count : 0;

                // Convert to world coordinates
                cv::Point2f new_world_pos = imageToWorld(img_center, avg_depth, image_size.width, image_size.height);

                // Update velocity (m/s)
                cv::Point2f new_velocity = (new_world_pos - obs.world_position) /
                                           ((current_frame - obs.last_seen) * time_per_frame);
                obs.velocity = 0.3f * new_velocity + 0.7f * obs.velocity;

                // Update position and history
                obs.position_history.push_back(new_world_pos);
                if (obs.position_history.size() > MAP_HISTORY) {
                    obs.position_history.pop_front();
                }

                obs.image_position = img_center;
                obs.world_position = new_world_pos;
                obs.average_depth = avg_depth;
                obs.last_seen = current_frame;
                obs.life_time++;

                // Draw on map
                drawObstacle(pair.first, obs);
            }
        }

        // Add new obstacles for unmatched clusters
        for (size_t i = 0; i < clusters.size(); ++i) {
            if (!matched_clusters[i]) {
                cv::Point2f img_center(0, 0);
                for (const auto& pt : clusters[i]) img_center += pt;
                img_center *= (1.0f / clusters[i].size());

                // Calculate average depth
                float depth_sum = 0;
                int depth_count = 0;
                for (const auto& pt : clusters[i]) {
                    int x = static_cast<int>(pt.x);
                    int y = static_cast<int>(pt.y);
                    if (x >= 0 && x < depth_map.cols && y >= 0 && y < depth_map.rows) {
                        depth_sum += depth_map.at<float>(y, x);
                        depth_count++;
                    }
                }
                float avg_depth = depth_count > 0 ? depth_sum / depth_count : 0;

                // Convert to world coordinates
                cv::Point2f world_pos = imageToWorld(img_center, avg_depth, image_size.width, image_size.height);

                Obstacle new_obs;
                new_obs.image_position = img_center;
                new_obs.world_position = world_pos;
                new_obs.velocity = cv::Point2f(0, 0);
                new_obs.average_depth = avg_depth;
                new_obs.life_time = 1;
                new_obs.last_seen = current_frame;
                new_obs.position_history.push_back(world_pos);

                obstacles[next_id++] = new_obs;

                // Draw on map
                drawObstacle(next_id - 1, new_obs);
            }
        }

        // Remove dead obstacles
        for (auto it = obstacles.begin(); it != obstacles.end(); ) {
            if (it->second.life_time <= 0) {
                it = obstacles.erase(it);
            } else {
                ++it;
            }
        }
    }

    void drawObstacle(int id, const Obstacle& obs) {
        cv::Point map_pos = worldToMap(obs.world_position);

        // Calculate distance from camera (Euclidean distance)
        float distance = std::sqrt(obs.world_position.x*obs.world_position.x +
                                   obs.world_position.y*obs.world_position.y);

        // Draw history trail with distance-based color gradient
        if (obs.position_history.size() > 1) {
            for (size_t i = 1; i < obs.position_history.size(); i++) {
                cv::Point prev = worldToMap(obs.position_history[i-1]);
                cv::Point curr = worldToMap(obs.position_history[i]);

                // Calculate distance at this history point
                float hist_dist = std::sqrt(obs.position_history[i].x*obs.position_history[i].x +
                                            obs.position_history[i].y*obs.position_history[i].y);

                // Color gradient from green (close) to red (far)
                int green = std::max(0, 255 - static_cast<int>(hist_dist * 20));
                int red = std::min(255, static_cast<int>(hist_dist * 20));
                cv::line(map_image, prev, curr, cv::Scalar(0, green, red), 2);
            }
        }

        // Draw obstacle with distance-based color
        int green = std::max(0, 255 - static_cast<int>(distance * 20));
        int red = std::min(255, static_cast<int>(distance * 20));
        cv::circle(map_image, map_pos, 7, cv::Scalar(0, green, red), -1);

        // Draw distance text
        std::stringstream dist_ss;
        dist_ss << std::fixed << std::setprecision(1) << distance << "m";
        std::string dist_text = dist_ss.str();

        // Draw ID and distance
        std::string label = std::to_string(id) + ": " + dist_text;
        cv::putText(map_image, label, map_pos + cv::Point(10, -10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);

        // Draw distance arc
        float angle = std::atan2(obs.world_position.x, obs.world_position.y);
        cv::ellipse(map_image, worldToMap(cv::Point2f(0,0)),
                    cv::Size(static_cast<int>(distance * MAP_SCALE),
                             static_cast<int>(distance * MAP_SCALE)),
                    0, 0, -angle * 180/M_PI, cv::Scalar(100, 100, 255), 1);
    }

    const std::unordered_map<int, Obstacle>& getObstacles() const {
        return obstacles;
    }

    const cv::Mat& getMapImage() const {
        return map_image;
    }
};

void processImage(fs::path &image_path, ModelType model_type = ModelType::DAD);

#endif //DRONE_NAVIGATION_IMAGE_PROCESSOR_HPP
