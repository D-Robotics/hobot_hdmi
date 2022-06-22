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

#include "include/image_display.h"

#include <stdarg.h>
#include <string>
#include <fstream>

#include "include/x3_vio_vot.h"
#include "rclcpp/rclcpp.hpp"
#include "include/video_utils.hpp"

extern "C" int ROS_printf(int nLevel, char *fmt, ...)
{
  char buf[512] = { 0 };
  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  switch (nLevel) {
    case 0:
      RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"), "%s", buf);
    break;
    case 1:
      RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"), "%s", buf);
    break;
    default:
      RCLCPP_INFO(rclcpp::get_logger("hobot_hdmi"), "%s", buf);
    break;
  }

  va_end(args);
}

int vot_param_init(x3_vot_info_t *vot_info, int nPicWidth, int nPicHeight)
{
  // devAttr
  vot_info->m_devAttr.enIntfSync = VOT_OUTPUT_1920x1080;
  vot_info->m_devAttr.u32BgColor = 0x108080;
  vot_info->m_devAttr.enOutputMode = HB_VOT_OUTPUT_BT1120;

  // layerAttr
  vot_info->m_stLayerAttr.stImageSize.u32Width = nPicWidth;  // 1920;
    vot_info->m_stLayerAttr.stImageSize.u32Height = nPicHeight;  // 1080;
  vot_info->m_stLayerAttr.big_endian = 0;
    vot_info->m_stLayerAttr.display_addr_type = 2;
    vot_info->m_stLayerAttr.display_addr_type_layer1 = 2;

  vot_info->m_stLayerAttr.dithering_flag = 0;
    vot_info->m_stLayerAttr.dithering_en = 0;
    vot_info->m_stLayerAttr.gamma_en = 0;
    vot_info->m_stLayerAttr.hue_en = 0;
    vot_info->m_stLayerAttr.sat_en = 0;
    vot_info->m_stLayerAttr.con_en = 0;
    vot_info->m_stLayerAttr.bright_en = 0;
    vot_info->m_stLayerAttr.theta_sign = 0;
    vot_info->m_stLayerAttr.contrast = 0;

    vot_info->m_stLayerAttr.theta_abs = 0;
    vot_info->m_stLayerAttr.saturation = 0;
    vot_info->m_stLayerAttr.off_contrast = 0;
    vot_info->m_stLayerAttr.off_bright = 0;

    vot_info->m_stLayerAttr.panel_type = 0;
    vot_info->m_stLayerAttr.rotate = 0;
    vot_info->m_stLayerAttr.user_control_disp = 0;

  // ChnAttr
  vot_info->m_stChnAttr.u32Priority = 2;
    vot_info->m_stChnAttr.s32X = 0;
    vot_info->m_stChnAttr.s32Y = 0;
    vot_info->m_stChnAttr.u32SrcWidth = nPicWidth;  // 1920;
    vot_info->m_stChnAttr.u32SrcHeight = nPicHeight;  // 1080;
    vot_info->m_stChnAttr.u32DstWidth = nPicWidth;  // 1920;
    vot_info->m_stChnAttr.u32DstHeight = nPicHeight;  // 1080;

  // cropAttr
  vot_info->m_cropAttrs.u32Width = vot_info->m_stChnAttr.u32DstWidth;
    vot_info->m_cropAttrs.u32Height = vot_info->m_stChnAttr.u32DstHeight;
  return 0;
}

ImageDisplay::ImageDisplay(const rclcpp::NodeOptions& node_options,
  std::string node_name, std::string topic_name)
    : Node(node_name, node_options) {
  this->declare_parameter("sub_img_topic", topic_name_);
  this->get_parameter("sub_img_topic", topic_name_);
  if (!topic_name.empty()) {
    topic_name_ = topic_name;
  }

  this->declare_parameter("io_method", _io_mode);
  get_parameter("io_method", _io_mode);
  RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"),
    "Create topic: %s,io=%s.", topic_name_.c_str(), _io_mode.c_str());
  if (0 != _io_mode.compare("shared_mem")) {
    subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
        topic_name_, 10,
        std::bind(&ImageDisplay::topic_callback, this, std::placeholders::_1));
  } else {
#ifdef USING_HBMEM
    hbmem_subscription_ =
        this->create_subscription_hbmem<hbm_img_msgs::msg::HbmMsg1080P>(
            topic_name_, 10,
            std::bind(&ImageDisplay::hbmem_topic_callback,
                      this, std::placeholders::_1));
    RCLCPP_WARN(rclcpp::get_logger("hobot_hdmi"),
      "Create hbmem_subscription with topic_name: %s, sub = %p",
       topic_name_.c_str(), hbmem_subscription_);
#endif
  }
}

ImageDisplay::~ImageDisplay() {
  x3_vot_deinit(&m_vot_info);
}

#include <sys/times.h>
// 返回ms
int32_t tool_calc_time_laps(
  const struct timespec &time_start, const struct timespec &time_end)
{
  int32_t nRetMs = 0;
  if (time_end.tv_nsec < time_start.tv_nsec)
  {
    nRetMs = (time_end.tv_sec - time_start.tv_sec - 1) * 1000 +
     (1000000000 + time_end.tv_nsec - time_start.tv_nsec) / 1000000;
  } else {
    nRetMs = (time_end.tv_sec - time_start.tv_sec) * 1000 +
             (time_end.tv_nsec - time_start.tv_nsec) / 1000000;
  }
  return nRetMs;
}

#ifdef USING_HBMEM
void ImageDisplay::hbmem_topic_callback(
    const hbm_img_msgs::msg::HbmMsg1080P::ConstSharedPtr msg) {
  // 判断格式，是 bgr8,nv12 ,rgb8
  m_MtxPutFrame.lock();
  if (0 == m_nVotSt) {
    m_nShowWidth = msg->width;
    m_nShowHeight = msg->height;
    vot_param_init(&m_vot_info, m_nShowWidth, m_nShowHeight);
    if (0 == x3_vot_init(&m_vot_info, m_nShowWidth, m_nShowHeight))
      m_nVotSt = 1;
  } else {
    int nNV12Y_sz = msg->width * msg->height;
    VOT_FRAME_INFO_S pstVotFrame;
    if (msg->width != m_nShowWidth ||
      msg->height != m_nShowHeight) {
        RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"), "[%s]->inW-h err %d-%d.",
          __func__, msg->width , msg->height);
        m_MtxPutFrame.unlock();
        return;
    }
    // 把yuv（1080P）数据发给vot显示
    // 数据结构转换
    memset(&pstVotFrame, 0, sizeof(VOT_FRAME_INFO_S));
    char tmpCoding[12] = {0};
    snprintf(tmpCoding, sizeof(tmpCoding), "%s", msg->encoding.data());
    if (0 == strcmp(tmpCoding, "bgr8") ||
      0 == strcmp(tmpCoding, "rgb8")) {
      if (msg->data_size != nNV12Y_sz * 3) {
        RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"), "[%s]->inLen err %d-%d.",
          __func__, msg->data_size, nNV12Y_sz * 3);
        m_MtxPutFrame.unlock();
        return;
      }
      if (nullptr == mPtrInNv12) {
        mPtrInNv12 = new uint8_t[nNV12Y_sz * 3 / 2];
      }
      if (0 == strcmp(tmpCoding, "bgr8")) {
        video_utils::BGR24_to_NV12(msg->data.data(), mPtrInNv12,
          msg->width, msg->height);
      } else if (0 == strcmp(tmpCoding, "rgb8")) {
        video_utils::RGB24_to_NV12(msg->data.data(), mPtrInNv12,
          msg->width, msg->height);
      }
      pstVotFrame.addr = mPtrInNv12;  // y分量虚拟地址
      pstVotFrame.addr_uv = mPtrInNv12 + nNV12Y_sz;  // uv分量虚拟地址
    } else {
      const uint8_t *pDataAdr = msg->data.data();
      pstVotFrame.addr = const_cast<uint8_t*>(pDataAdr);  // y分量虚拟地址
      // uv分量虚拟地址
      pstVotFrame.addr_uv = const_cast<uint8_t*>(pDataAdr + nNV12Y_sz);
    }
    pstVotFrame.size = nNV12Y_sz * 3 / 2;
    // 发送数据帧到vo模块
    x3_vot_sendframe(&pstVotFrame);
  }
  m_MtxPutFrame.unlock();
  return;
}
#endif

void ImageDisplay::topic_callback(
       const sensor_msgs::msg::Image::ConstSharedPtr msg) {
  // 判断格式，是 bgr8,nv12 ,rgb8
  m_MtxPutFrame.lock();
  if (0 == m_nVotSt) {
    m_nShowWidth = msg->width;
    m_nShowHeight = msg->height;
    vot_param_init(&m_vot_info, m_nShowWidth, m_nShowHeight);
    if (0 == x3_vot_init(&m_vot_info, m_nShowWidth, m_nShowHeight))
      m_nVotSt = 1;
  } else {
    VOT_FRAME_INFO_S pstVotFrame;
    int nNV12Y_sz = msg->width * msg->height;
    if (msg->width != m_nShowWidth ||
      msg->height != m_nShowHeight) {
        RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"), "[%s]->inWh err %d-%d.",
          __func__, msg->width , msg->height);
        m_MtxPutFrame.unlock();
        return;
    }
    // 把yuv（1080P）数据发给vot显示
    // 数据结构转换
    memset(&pstVotFrame, 0, sizeof(VOT_FRAME_INFO_S));
    if (0 == strcmp(msg->encoding.data(), "bgr8") ||
      0 == strcmp(msg->encoding.data(), "rgb8")) {
      if (msg->data.size() != nNV12Y_sz * 3) {
        RCLCPP_ERROR(rclcpp::get_logger("hobot_hdmi"), "[%s]->inL err %d-%d.",
          __func__, msg->data.size(), nNV12Y_sz * 3);
        m_MtxPutFrame.unlock();
        return;
      }
      if (nullptr == mPtrInNv12) {
        mPtrInNv12 = new uint8_t[nNV12Y_sz * 3 / 2];
      }
      if (0 == strcmp(msg->encoding.data(), "bgr8")) {
        video_utils::BGR24_to_NV12(msg->data.data(), mPtrInNv12,
          msg->width, msg->height);
      } else if (0 == strcmp(msg->encoding.data(), "rgb8")) {
        video_utils::RGB24_to_NV12(msg->data.data(), mPtrInNv12,
          msg->width, msg->height);
      }
      pstVotFrame.addr = mPtrInNv12;  // y分量虚拟地址
      pstVotFrame.addr_uv = mPtrInNv12 + nNV12Y_sz;  // uv分量虚拟地址
    } else {
      const uint8_t *pDataAdr = msg->data.data();
      pstVotFrame.addr = const_cast<uint8_t*>(pDataAdr);  // y分量虚拟地址
      // uv分量虚拟地址
      pstVotFrame.addr_uv = const_cast<uint8_t*>(pDataAdr + nNV12Y_sz);
    }
    pstVotFrame.size = nNV12Y_sz * 3 / 2;
    // 发送数据帧到vo模块
    x3_vot_sendframe(&pstVotFrame);
  }
  m_MtxPutFrame.unlock();
  return;
}
