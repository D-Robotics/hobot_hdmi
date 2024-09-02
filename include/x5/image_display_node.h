// Copyright (c) 2024，D-Robotics.
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

#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <queue>

#include "ai_msgs/msg/perception_targets.hpp"
#ifdef SHARED_MEM_ENABLED
#include "hbm_img_msgs/msg/hbm_msg1080_p.hpp"
#endif
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

#include "plugin_framework.h"
#include "display_framework.h"

#ifndef HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_
#define HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_

using rclcpp::NodeOptions;

struct compare_frame {
  bool operator()(const sensor_msgs::msg::Image::SharedPtr f1,
                  const sensor_msgs::msg::Image::SharedPtr f2) {
    return ((f1->header.stamp.sec > f2->header.stamp.sec) ||
            ((f1->header.stamp.sec == f2->header.stamp.sec) &&
             (f1->header.stamp.nanosec > f2->header.stamp.nanosec)));
  }
};
struct compare_msg {
  bool operator()(const ai_msgs::msg::PerceptionTargets::SharedPtr m1,
                  const ai_msgs::msg::PerceptionTargets::SharedPtr m2) {
    return ((m1->header.stamp.sec > m2->header.stamp.sec) ||
            ((m1->header.stamp.sec == m2->header.stamp.sec) &&
             (m1->header.stamp.nanosec > m2->header.stamp.nanosec)));
  }
};

class ImageDisplay : public rclcpp::Node {
 public:
  ImageDisplay(const rclcpp::NodeOptions & node_options = NodeOptions(),
   std::string node_name = "img_sub", std::string topic_name = "");
  ~ImageDisplay();

  int FeedFromLocal();

  int Run();

  int DisplayFrame(const sensor_msgs::msg::Image::SharedPtr img_msg,
                    float &ratio_h, float &ratio_w);
  int DisplaySmartMsg(const ai_msgs::msg::PerceptionTargets::SharedPtr ai_msg,
                      const float ratio_h, const float ratio_w);

 private:

  std::string ros_img_sub_topic_name_ = "/image";
  rclcpp::Subscription<sensor_msgs::msg::Image>::ConstSharedPtr
      ros_img_subscription_ = nullptr;
  void RosImgProcess(const sensor_msgs::msg::Image::ConstSharedPtr msg);

  std::string ai_msg_sub_topic_name_ = "/hobot_detection";
  rclcpp::Subscription<ai_msgs::msg::PerceptionTargets>::SharedPtr 
      ai_img_subscription_ = nullptr;
  void SmartMsgProcess(const ai_msgs::msg::PerceptionTargets::SharedPtr msg);

#ifdef SHARED_MEM_ENABLED
  rclcpp::Subscription<hbm_img_msgs::msg::HbmMsg1080P>::ConstSharedPtr
      sharedmem_img_subscription_ = nullptr;
  std::string sharedmem_img_topic_name_ = "/hbmem_img";
  void SharedMemImgProcess(
      const hbm_img_msgs::msg::HbmMsg1080P::ConstSharedPtr msg);
#endif

  std::shared_ptr<DisplayFramework> display_framework_ = nullptr;
  bool is_shared_mem_ = false;
  std::string drm_config_file_ = "config/display_framework/mono.json";
  bool only_show_image_ = true;

  std::shared_ptr<std::thread> predict_task_ = nullptr;

  std::mutex map_smart_mutex_;
  std::condition_variable map_smart_condition_;

  std::priority_queue<sensor_msgs::msg::Image::SharedPtr,
                    std::vector<sensor_msgs::msg::Image::SharedPtr>,
                    compare_frame>
    frames_;
  std::priority_queue<ai_msgs::msg::PerceptionTargets::SharedPtr,
                    std::vector<ai_msgs::msg::PerceptionTargets::SharedPtr>,
                    compare_msg>
    smart_msg_;
};

#endif  // HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_
