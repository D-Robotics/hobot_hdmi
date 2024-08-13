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

#include <cv_bridge/cv_bridge.h>

#include "image_util.h"
#include "image_display_node.h"


ImageDisplay::ImageDisplay(const rclcpp::NodeOptions& node_options,
  std::string node_name, std::string topic_name)
    : Node(node_name, node_options) {
  this->declare_parameter<bool>("is_shared_mem", is_shared_mem_);
  this->declare_parameter<std::string>("ros_img_sub_topic_name",
                                       ros_img_sub_topic_name_);

  this->get_parameter<bool>("is_shared_mem", is_shared_mem_);
  this->get_parameter<std::string>("ros_img_sub_topic_name",
                                   ros_img_sub_topic_name_);

  std::stringstream ss;
  ss << "Parameter:"
     << "\n is_shared_mem: " << is_shared_mem_
     << "\n ros_img_sub_topic_name: " << ros_img_sub_topic_name_;
  RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"), "%s", ss.str().c_str());

  display_framework_ = std::make_shared<DisplayFramework>(drm_config_file_);

  predict_task_ = std::make_shared<std::thread>(
      std::bind(&ImageDisplay::Run, this));
  timer_ = create_wall_timer(std::chrono::milliseconds(20),
                                std::bind(&ImageDisplay::Display, this));
  FeedFromLocal();

  if (is_shared_mem_) {
#ifdef SHARED_MEM_ENABLED
  RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"),
              "Create hbmem_subscription with topic_name: %s",
              sharedmem_img_topic_name_.c_str());
  sharedmem_img_subscription_ =
      this->create_subscription<hbm_img_msgs::msg::HbmMsg1080P>(
          sharedmem_img_topic_name_,
          rclcpp::SensorDataQoS(),
          std::bind(&ImageDisplay::SharedMemImgProcess,
                    this,
                    std::placeholders::_1));
#else
  RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"), "Unsupport shared mem");
#endif
  } else {
  RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"),
              "Create subscription with topic_name: %s",
              ros_img_sub_topic_name_.c_str());
  ros_img_subscription_ =
      this->create_subscription<sensor_msgs::msg::Image>(
          ros_img_sub_topic_name_,
          10,
          std::bind(
              &ImageDisplay::RosImgProcess, this, std::placeholders::_1));
  }
}

ImageDisplay::~ImageDisplay() {
  if (predict_task_ && predict_task_->joinable()) {
    predict_task_->join();
    predict_task_.reset();
  }
  if (timer_ != nullptr){
    timer_->cancel();
  }
}

#ifdef SHARED_MEM_ENABLED
void ImageDisplay::SharedMemImgProcess(
    const hbm_img_msgs::msg::HbmMsg1080P::ConstSharedPtr img_msg) {
  if (!img_msg || !rclcpp::ok()) {
    return;
  }

  std::stringstream ss;
  ss << "Recved img encoding: "
     << std::string(reinterpret_cast<const char*>(img_msg->encoding.data()))
     << ", h: " << img_msg->height << ", w: " << img_msg->width
     << ", step: " << img_msg->step << ", index: " << img_msg->index
     << ", stamp: " << img_msg->time_stamp.sec << "_"
     << img_msg->time_stamp.nanosec << ", data size: " << img_msg->data_size;
  RCLCPP_INFO(rclcpp::get_logger("hobot_hdmi"), "%s", ss.str().c_str());

  if ("nv12" ==
      std::string(reinterpret_cast<const char *>(img_msg->encoding.data()))) {
    auto nv12_data = reinterpret_cast<const uint8_t *>(img_msg->data.data());
    int ret = 0;
    if (img_msg->height != 1080 || img_msg->width != 1920) {
      cv::Mat nv12;
      float ratio_h;
      float ratio_w;
      ResizeNV12Img(reinterpret_cast<const char *>(img_msg->data.data()), img_msg->height, img_msg->width, 1080, 1920, ratio_h, ratio_w, nv12);
      ret = display_framework_->FillBuffer(33, const_cast<uint8_t*>(nv12.data));
    } else {
      ret = display_framework_->FillBuffer(33, const_cast<uint8_t*>(nv12_data));
    }

    if(ret != 0) {
      RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"),
            "Fill buffer failed!");
    }
  } else {
    RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"),
                 "Unsupported img encoding: %s, only nv12 img encoding is "
                 "supported for shared mem.",
                 img_msg->encoding.data());
    return;
  }
}
#endif

void ImageDisplay::RosImgProcess(
    const sensor_msgs::msg::Image::ConstSharedPtr img_msg) {
  if (!img_msg) {
    RCLCPP_DEBUG(rclcpp::get_logger("hobot_hdmi"), "Get img failed");
    return;
  }

  if (!rclcpp::ok()) {
    return;
  }

  std::stringstream ss;
  ss << "Recved img encoding: " << img_msg->encoding
     << ", h: " << img_msg->height << ", w: " << img_msg->width
     << ", step: " << img_msg->step
     << ", frame_id: " << img_msg->header.frame_id
     << ", stamp: " << img_msg->header.stamp.sec << "_"
     << img_msg->header.stamp.nanosec
     << ", data size: " << img_msg->data.size();
  RCLCPP_INFO(rclcpp::get_logger("hobot_hdmi"), "%s", ss.str().c_str());

  char *nv12_data;
  if ("rgb8" == img_msg->encoding) {
    auto cv_img =
        cv_bridge::cvtColorForDisplay(cv_bridge::toCvShare(img_msg), "bgr8");
    cv::Mat bgr_mat = cv_img->image;
    cv::cvtColor(bgr_mat, bgr_mat, cv::COLOR_BGR2RGB);
    cv::Mat nv12_mat;
    BGRToNv12(bgr_mat, nv12_mat);
    nv12_data = reinterpret_cast<char*>(nv12_mat.data);
  } else if ("bgr8" == img_msg->encoding) {
    auto cv_img =
        cv_bridge::cvtColorForDisplay(cv_bridge::toCvShare(img_msg), "bgr8");
    cv::Mat bgr_mat = cv_img->image;
    cv::Mat nv12_mat;
    BGRToNv12(bgr_mat, nv12_mat);
    nv12_data = reinterpret_cast<char*>(nv12_mat.data);
  } else if ("nv12" == img_msg->encoding) {  // nv12格式使用hobotcv resize
    nv12_data = nv12_data = reinterpret_cast<char*>(const_cast<unsigned char*>(img_msg->data.data()));
  }

  int ret = 0;
  if (img_msg->height != 1080 || img_msg->width != 1920) {
    cv::Mat nv12;
    float ratio_h;
    float ratio_w;
    ResizeNV12Img(nv12_data, img_msg->height, img_msg->width, 1080, 1920, ratio_h, ratio_w, nv12);
    ret = display_framework_->FillBuffer(33, reinterpret_cast<uint8_t*>(nv12.data));
  } else {
    ret = display_framework_->FillBuffer(33, reinterpret_cast<uint8_t*>(nv12_data));
  }

  if(ret != 0) {
    RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"),
          "Fill buffer failed!");
  }
}

int ImageDisplay::FeedFromLocal() {
  uint8_t *nv12_data;
  readbinary("config/nv12_1920x1080.yuv", nv12_data);
  int ret = display_framework_->FillBuffer(33, nv12_data);
  if(ret != 0) {
    RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"),
        "Fill buffer failed!");
  }
  std::free(nv12_data);

  uint8_t *rgb_data = static_cast<uint8_t*>(std::malloc(512 * 512 * 3));
  for (size_t i = 0; i < 512 * 256; ++i) {
      rgb_data[i * 3] = 0;   // 红色通道
      rgb_data[i * 3 + 1] = 255; // 绿色通道
      rgb_data[i * 3 + 2] = 255; // 蓝色通道
  } 
  for (size_t i = 512 * 256; i < 512 * 512; ++i) {
      rgb_data[i * 3] = 255;   // 红色通道
      rgb_data[i * 3 + 1] = 0; // 绿色通道
      rgb_data[i * 3 + 2] = 0; // 蓝色通道
  }
  // ret = display_framework_->FillBuffer(40, rgb_data);
  if(ret != 0) {
    RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"),
        "Fill buffer failed!");
  }
  std::free(rgb_data);
 return 0; 
}

int ImageDisplay::Run() {
  return 0;
}

int ImageDisplay::Display() {
  return display_framework_->Run();
}