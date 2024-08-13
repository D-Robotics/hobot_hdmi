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

#ifdef SHARED_MEM_ENABLED
#include "hbm_img_msgs/msg/hbm_msg1080_p.hpp"
#endif
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

#include "display_framework.h"

#ifndef HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_
#define HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_

using rclcpp::NodeOptions;

class ImageDisplay : public rclcpp::Node {
 public:
  ImageDisplay(const rclcpp::NodeOptions & node_options = NodeOptions(),
   std::string node_name = "img_sub", std::string topic_name = "");
  ~ImageDisplay();

  int FeedFromLocal();

  int Run();

  int Display();

 private:

  // 目前只支持订阅深度图原图
  std::string ros_img_sub_topic_name_ = "/image";
  rclcpp::Subscription<sensor_msgs::msg::Image>::ConstSharedPtr
      ros_img_subscription_ = nullptr;
  void RosImgProcess(const sensor_msgs::msg::Image::ConstSharedPtr msg);

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

  std::shared_ptr<std::thread> predict_task_ = nullptr;

  rclcpp::TimerBase::SharedPtr timer_;
};

#endif  // HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_
