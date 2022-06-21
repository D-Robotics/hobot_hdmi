// Copyright (c) 2022，Horizon Robotics.
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


#include <string>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"

#ifdef USING_HBMEM
#include "hbm_img_msgs/msg/hbm_msg1080_p.hpp"
#endif

#include "include/x3_vio_vot.h"

#ifndef HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_
#define HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_

using rclcpp::NodeOptions;
using ImgCbType =
  std::function<void(const sensor_msgs::msg::Image::ConstSharedPtr &msg)>;

class ImageDisplay : public rclcpp::Node {
 public:
  ImageDisplay(const rclcpp::NodeOptions & node_options = NodeOptions(),
   std::string node_name = "img_sub", std::string topic_name = "");
  ~ImageDisplay();

 private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::ConstSharedPtr
    subscription_ = nullptr;

  // 目前只支持订阅原图，可以使用压缩图"/image_raw/compressed" topic
  // 和sensor_msgs::msg::CompressedImage格式扩展订阅压缩图
  std::string topic_name_ = "/image_raw";
  std::string _io_mode = "ros";
#ifdef USING_HBMEM
  rclcpp::SubscriptionHbmem<hbm_img_msgs::msg::HbmMsg1080P>::SharedPtr
      hbmem_subscription_;
#endif

  std::chrono::high_resolution_clock::time_point sub_img_tp_;
  int sub_img_frameCount_ = 0;
  std::mutex frame_stat_mtx_;
  std::mutex          m_MtxPutFrame;
  int m_nShowWidth;
  int m_nShowHeight;

  // latency
  // 滑动窗口测方差 ， 20 s ，标准帧率 ,只测试原始图
  std::vector<int> m_vecFps;
  std::vector<int> m_vecLatency;

  std::chrono::high_resolution_clock::time_point sub_imghbm_tp_;
  int sub_imghbm_frameCount_ = 0;
  std::mutex frame_stathbm_mtx_;
  std::vector<int> m_vecHbmFps;
  std::vector<int> m_vecHbmLatency;
  int m_nVotSt = 0;  // 0, noinit; 1, init
  uint8_t *mPtrInNv12 = nullptr;
  x3_vot_info_t m_vot_info;  // x3的视频输出（vo、iar）配置

  void topic_callback(const sensor_msgs::msg::Image::ConstSharedPtr msg);
#ifdef USING_HBMEM
  void hbmem_topic_callback(
    const hbm_img_msgs::msg::HbmMsg1080P::ConstSharedPtr msg);
#endif
};

#endif  // HOBOT_HDMI_INCLUDE_IMAGE_DISPLAY_H_
