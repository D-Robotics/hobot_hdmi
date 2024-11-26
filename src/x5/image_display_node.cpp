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
  this->declare_parameter<bool>("only_show_image", only_show_image_);
  this->declare_parameter<std::string>("ros_img_sub_topic_name",
                                       ros_img_sub_topic_name_);
  this->declare_parameter<std::string>("ai_msg_sub_topic_name",
                                       ai_msg_sub_topic_name_);

  this->get_parameter<bool>("is_shared_mem", is_shared_mem_);
  this->get_parameter<bool>("only_show_image", only_show_image_);
  this->get_parameter<std::string>("ros_img_sub_topic_name",
                                   ros_img_sub_topic_name_);
  this->get_parameter<std::string>("ai_msg_sub_topic_name",
                                   ai_msg_sub_topic_name_);

  std::stringstream ss;
  ss << "Parameter:"
     << "\n is_shared_mem: " << is_shared_mem_
     << "\n only_show_image: " << only_show_image_
     << "\n ai_msg_sub_topic_name: " << ai_msg_sub_topic_name_
     << "\n ros_img_sub_topic_name: " << ros_img_sub_topic_name_;
  RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"), "%s", ss.str().c_str());

  if (only_show_image_) {
    drm_config_file_ = "config/display_framework/mono.json";
  } else {
    drm_config_file_ = "config/display_framework/mono_with_render.json";
  }
  display_framework_ = std::make_shared<DisplayFramework>(drm_config_file_);
  display_framework_->Run();

  predict_task_ = std::make_shared<std::thread>(
      std::bind(&ImageDisplay::Run, this));
  FeedFromLocal();

  if (!only_show_image_) {
    ai_img_subscription_ = this->create_subscription<ai_msgs::msg::PerceptionTargets>(
        ai_msg_sub_topic_name_,
        10,
        std::bind(&ImageDisplay::SmartMsgProcess, this, std::placeholders::_1));
  }
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
  {
    std::unique_lock<std::mutex> lock(map_smart_mutex_);
    while (!frames_.empty()) {
      frames_.pop();
    }
  }
  {
    std::unique_lock<std::mutex> lock(map_smart_mutex_);
    while (!smart_msg_.empty()) {
      smart_msg_.pop();
    }
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

  auto image = std::make_shared<sensor_msgs::msg::Image>();
  image->header.stamp = img_msg->time_stamp;
  image->header.frame_id = img_msg->index;
  image->encoding = "nv12";
  image->height = img_msg->height;
  image->width = img_msg->width;
  image->data.resize(img_msg->data_size);
  memcpy(image->data.data(), img_msg->data.data(), img_msg->data_size);

  {
    std::unique_lock<std::mutex> lock(map_smart_mutex_);
    frames_.push(image);
    if (frames_.size() > 100) {
      frames_.pop();
      RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"),
                  "hdmi has cache image num > 100, drop the oldest "
                  "image message");
    }
    map_smart_condition_.notify_one();
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

  auto image = std::make_shared<sensor_msgs::msg::Image>();
  image->header = img_msg->header;
  image->encoding = img_msg->encoding;
  image->height = img_msg->height;
  image->width = img_msg->width;
  image->data.resize(img_msg->data.size());
  memcpy(image->data.data(), img_msg->data.data(), img_msg->data.size());

  {
    std::unique_lock<std::mutex> lock(map_smart_mutex_);
    frames_.push(image);
    if (frames_.size() > 100) {
      frames_.pop();
      RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"),
                  "hdmi has cache image num > 100, drop the oldest "
                  "image message");
    }
    map_smart_condition_.notify_one();
  }
}

void ImageDisplay::SmartMsgProcess(
    const ai_msgs::msg::PerceptionTargets::SharedPtr msg) {
  
  std::stringstream ss;
  ss << "Recved Smart msg: " <<  msg->header.frame_id
     << ", stamp: " << msg->header.stamp.sec << "_"
     << msg->header.stamp.nanosec << ", data size: " << msg->targets.size();
  RCLCPP_INFO(rclcpp::get_logger("hobot_hdmi"), "%s", ss.str().c_str());

  {
    std::unique_lock<std::mutex> lock(map_smart_mutex_);
    smart_msg_.push(msg);
    if (smart_msg_.size() > 100) {
      smart_msg_.pop();
      RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"),
                  "hdmi has cache smart message num > 100, drop the "
                  "oldest smart message");
    }
    map_smart_condition_.notify_one();
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

  // cv::Mat img(1080, 1920, CV_8UC4, cv::Scalar(255, 0, 255, 255));
  // // 定义矩形框的位置和大小
  // cv::Rect rect(128, 128, 256, 256);  // 左上角坐标(128, 128)，宽256，高256
  // // 定义框的颜色（A, B, G, R），例如红色，透明度为128
  // cv::Scalar boxColor(128, 0, 0, 255);
  // // 在图像上绘制矩形框
  // cv::rectangle(img, rect, boxColor, 5);  // -1 表示填充矩形
  // ret = display_framework_->FillBuffer(40, img.data);

  return 0; 
}

int ImageDisplay::Run() {
  while (rclcpp::ok()) {
    std::unique_lock<std::mutex> lock(map_smart_mutex_);
    map_smart_condition_.wait(lock);

    display_framework_->Run();
    if (only_show_image_) {
      while (!frames_.empty()) {
        auto frame = frames_.top();
        lock.unlock();
        float ratio_h;
        float ratio_w;
        DisplayFrame(frame, ratio_h, ratio_w);
        lock.lock();
        frames_.pop();
      }
    } else {
      while (!smart_msg_.empty() && !frames_.empty()) {
        auto msg = smart_msg_.top();
        auto frame = frames_.top();
        if (msg->header.stamp == frame->header.stamp) {
          lock.unlock();
          float ratio_h;
          float ratio_w;
          DisplayFrame(frame, ratio_h, ratio_w);
          DisplaySmartMsg(msg, ratio_h, ratio_w);
          lock.lock();
          smart_msg_.pop();
          frames_.pop();
        } else if ((msg->header.stamp.sec > frame->header.stamp.sec) ||
                  ((msg->header.stamp.sec == frame->header.stamp.sec) &&
                    (msg->header.stamp.nanosec >
                    frame->header.stamp.nanosec))) {
          frames_.pop();
        } else {
          smart_msg_.pop();
        }
      }
    }
  }
  return 0;
}

int ImageDisplay::DisplayFrame(const sensor_msgs::msg::Image::SharedPtr img_msg,
                            float &ratio_h, float &ratio_w) {

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
    nv12_data = reinterpret_cast<char*>(const_cast<unsigned char*>(img_msg->data.data()));
  }

  int ret = 0;
  if (img_msg->height != 1080 || img_msg->width != 1920) {
    cv::Mat nv12;
    ResizeNV12Img(nv12_data, img_msg->height, img_msg->width, 1080, 1920, ratio_h, ratio_w, nv12);
    ret = display_framework_->FillBuffer(33, reinterpret_cast<uint8_t*>(nv12.data));
  } else {
    ret = display_framework_->FillBuffer(33, reinterpret_cast<uint8_t*>(nv12_data));
  }

  if(ret != 0) {
    RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"),
          "Fill buffer failed!");
  }
  return ret;
}

int ImageDisplay::DisplaySmartMsg(const ai_msgs::msg::PerceptionTargets::SharedPtr ai_msg,
                                  const float ratio_h, const float ratio_w) {
  cv::Mat img(1080, 1920, CV_8UC4, cv::Scalar(0, 0, 0, 0));
  SegPlugin::RenderSeg(img, ai_msg);
  KPSPlugin::RenderKPS(img, ai_msg, ratio_h, ratio_w);
  DetPlugin::RenderDet(img, ai_msg, ratio_h, ratio_w);
  int ret = display_framework_->FillBuffer(40, img.data);
  return ret;
}