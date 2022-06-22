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

#ifndef HOBOT_HDMI_INCLUDE_X3_VIO_VOT_H_
#define HOBOT_HDMI_INCLUDE_X3_VIO_VOT_H_

#include "vio/hb_common_vot.h"
#include "vio/hb_vot.h"

// 定义x3输出通道的配置结构体
typedef struct {
    VOT_VIDEO_LAYER_ATTR_S m_stLayerAttr;
    VOT_CHN_ATTR_S m_stChnAttr;
    VOT_WB_ATTR_S m_stWbAttr;
    VOT_CROP_INFO_S m_cropAttrs;
    VOT_PUB_ATTR_S m_devAttr;
} x3_vot_info_t;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
int ROS_printf(int nLevel, char *fmt, ...);
int x3_vot_sendframe(VOT_FRAME_INFO_S *out_frame);
int x3_vot_init(x3_vot_info_t*, int nWidth, int nHeight);
int x3_vot_deinit(x3_vot_info_t*);

#ifdef __cplusplus
};
#endif  /* __cplusplus */
#endif  // HOBOT_HDMI_INCLUDE_X3_VIO_VOT_H_
