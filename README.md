# Optimal Drone Navigation Based on Visual Obstacle Analysis

Authors (team): [Anastasiia Pelekh](https://github.com/Drakonchyk), [Ksenia Kretsula](https://github.com/kretsulaksusha).

## Description

Autonomous drone navigation presents significant challenges, particularly in environments with complex obstacles and limited sensing capabilities. This research explores a conceptual system for real-time obstacle detection and avoidance using only a single onboard camera. Our approach leverages monocular vision combined with feature point detection to estimate depth and identify obstacles dynamically. By applying advanced computer vision techniques, the system processes visual input to detect, classify, and respond to obstacles in real time, enabling intelligent navigation based purely on visual data. This theoretical framework provides insights into the feasibility and potential of vision-based drone navigation without reliance on additional hardware sensors.

## Prerequisites

- GCC
- CMake
- CV

### Installation

```shell
git clone https://github.com/kretsulaksusha/drove_navigation.git
cd drove_navigation
```

### Compilation

```shell
./compile.sh -R
```

### Usage

Create directory `models` and download the model: [Midas GitHub: model-small.onnx](https://github.com/isl-org/MiDaS/releases/download/v2_1/model-small.onnx) for depth estimation.

```shell
mkdir -p models
wget -P ./models https://github.com/isl-org/MiDaS/releases/download/v2_1/model-small.onnx
```

### Results

### Sources

- []()