// Copyright (c) 2024，D-Robotics..
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PLUGIN_FRAMEWORK_H_
#define PLUGIN_FRAMEWORK_H_

#include <iostream>
#include <map>
// #include <memory>
#include <string>
#include <vector>

#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include "ai_msgs/msg/attribute.hpp"
#include "ai_msgs/msg/perception_targets.hpp"
#include "geometry_msgs/msg/point32.hpp"
#include "sensor_msgs/msg/region_of_interest.hpp"


static uint8_t bgr_putpalette[] = {
    0,   0 ,  0  , 244, 35 , 232, 70 , 70 , 70 , 102, 102, 156, 190, 153, 153, 
    153, 153, 153, 250, 170, 30 , 220, 220, 0  , 107, 142, 35 , 152, 251, 152, 
    0  , 130, 180, 220, 20 , 60 , 255, 0  , 0  , 0  , 0  , 142, 0  , 0  , 70 , 
    0  , 60 , 100, 0  , 80 , 100, 0  , 0  , 230, 119, 11 , 32 , 216, 191, 69 , 
    50 , 33 , 199, 108, 59 , 247, 249, 96 , 97 , 97 , 234, 195, 239, 202, 156, 
    81 , 177, 90 , 180, 100, 245, 251, 146, 184, 245, 26 , 209, 56 , 20 , 144, 
    210, 56 , 241, 19 , 75 , 171, 144, 17 , 198, 216, 105, 125, 108, 212, 181, 
    75 , 189, 225, 137, 152, 226, 210, 107, 81 , 130, 189, 63 , 4  , 31 , 139, 
    106, 202, 255, 184, 64 , 56 , 200, 69 , 31 , 62 , 129, 13 , 19 , 235, 0  , 
    255, 129, 8  , 238, 24 , 80 , 176, 115, 54 , 232, 100, 164, 13 , 192, 234, 
    48 , 140, 176, 178, 145, 83 , 115, 225, 250, 18 , 6  , 98 , 34 , 156, 78 , 
    74 , 120, 22 , 185, 5  , 159, 111, 133, 243, 170, 252, 118, 23 , 29 , 143, 
    237, 6  , 163, 104, 231, 87 , 18 , 15 , 185, 45 , 152, 178, 147, 116, 56 , 
    28 , 197, 148, 134, 46 , 205, 243, 200, 47 , 5  , 233, 70 , 224, 88 , 0  , 
    237, 82 , 6  , 180, 104, 75 , 80 , 91 , 20 , 95 , 225, 61 , 91 , 37 , 187, 
    129, 183, 114, 246, 21 , 181, 26 , 90 , 201, 218, 8  , 81 , 97 , 14 , 208, 
    51 , 172, 247
};

static cv::Scalar box_color = cv::Scalar(255, 178, 194, 18);

// 定义框的颜色（A, B, G, R），例如红色，透明度为128
static std::vector<cv::Scalar> kps_colors{
    cv::Scalar(255, 255, 211, 37), // light blue
    cv::Scalar(255, 0, 0, 255),    // red
    cv::Scalar(255, 0, 165, 255),  // orange
    cv::Scalar(255, 0, 255, 255),  // yellow
    cv::Scalar(255, 14, 255, 14),  // green
    cv::Scalar(255, 255, 0, 0),    // blue
    cv::Scalar(255, 144, 238, 144),// light green
    cv::Scalar(255, 217, 22, 223),// pink
};

static std::map<int, std::string> gesture_map{{0, ""},
                                       {1, "Finger Heart"},
                                       {2, "Thumb Up"},
                                       {3, "Victory"},
                                       {4, "Mute"},
                                       {5, "Palm"},
                                       {6, "IndexFingerAntiClockwise"},
                                       {7, "IndexFingerClockwise"},
                                       {8, "Pinch"},
                                       {9, "Palmpat"},
                                       {10, "Palm Move"},
                                       {11, "Okay"},
                                       {12, "ThumbLeft"},
                                       {13, "ThumbRight"},
                                       {14, "Awesome"},
                                       {15, "PinchMove"},
                                       {16, "PinchAntiClockwise"},
                                       {17, "PinchClockwise"}};


class RenderPlugin {
 public:
  RenderPlugin(int img_height = 1080, int img_width = 1920) 
    : img_height(img_height), img_width(img_width) {}
  ~RenderPlugin() {}

  int img_height;
  int img_width;
};

class AttributesPlugin : RenderPlugin{
 public:
  AttributesPlugin(int img_height = 1080, int img_width = 1920) {}
  ~AttributesPlugin() {}
 
 int32_t RenderAttributes(cv::Mat &mat,
    const std::vector<ai_msgs::msg::Attribute> &attributes,
    const sensor_msgs::msg::RegionOfInterest &rect,
    const std::string &type,
    const uint64 &track_id);

 private:
  int x_stride = 17;
  int y_stride = 30;
};

class SegPlugin : RenderPlugin{
 public:
  SegPlugin(int img_height = 1080, int img_width = 1920) {}
  ~SegPlugin() {}
 
  static int32_t RenderSeg(cv::Mat &mat,
                    const ai_msgs::msg::PerceptionTargets::SharedPtr &ai_msg);
};

class DetPlugin : RenderPlugin{
 public:
  DetPlugin(int img_height = 1080, int img_width = 1920) {}
  ~DetPlugin() {}

  static int32_t RenderDet(cv::Mat &mat,
                    const ai_msgs::msg::PerceptionTargets::SharedPtr &ai_msg,
                    float ratio_h,
                    float ratio_w);
};

class KPSPlugin : RenderPlugin{
 public:
  KPSPlugin(int img_height = 1080, int img_width = 1920) {}
  ~KPSPlugin() {}

  static int32_t RenderKPS(cv::Mat &mat,
                const ai_msgs::msg::PerceptionTargets::SharedPtr &ai_msg,
                const float ratio_h,
                const float ratio_w);
};

#endif  // PLUGIN_FRAMEWORK_H_