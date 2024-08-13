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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

#include "vio/hb_vot.h"
#include "vio/hb_vio_interface.h"
#include "vio/hb_sys.h"

#include "x3_vio_vot.h"

int x3_vot_sendframe(VOT_FRAME_INFO_S *out_frame)
{
    int ret = 0;
    ret = HB_VOT_SendFrame(0, 0, out_frame, -1);

    return ret;
}

int x3_vot_init(x3_vot_info_t *vot_info, int nWidth, int nHeight)
{
    int ret = 0;

    ret = HB_VOT_SetPubAttr(0, &vot_info->m_devAttr);
    if (ret) {
        ROS_printf(0, "HB_VOT_SetPubAttr failed\n");
        goto err;
    }

    ret = HB_VOT_Enable(0);
    if (ret) {
        ROS_printf(0, "HB_VOT_Enable failed.\n");
        goto err;
    }

    VOT_VIDEO_LAYER_ATTR_S stLayerAttr;
    ret = HB_VOT_GetVideoLayerAttr(0, &stLayerAttr);
    if (ret) {
    ROS_printf(0, "HB_VOT_GetVideoLayerAttr failed.\n");
        goto err;
    }
    vot_info->m_stLayerAttr = stLayerAttr;
    ROS_printf(2, "stLayer width:%d\n", stLayerAttr.stImageSize.u32Width);
    ROS_printf(2, "stLayer height:%d\n", stLayerAttr.stImageSize.u32Height);
    stLayerAttr.user_control_disp = 1;
    ret = HB_VOT_SetVideoLayerAttr(0, &stLayerAttr);
    if (ret) {
        ROS_printf(0, "HB_VOT_SetVideoLayerAttr failed.\n");
        goto err;
    }

    // 缩放 begin
    if (nWidth != 1920 || nHeight != 1080) {
        VOT_UPSCALE_ATTR_S stUpScale;
        ret = HB_VOT_GetVideoLayerUpScale(0, &stUpScale);
        if (ret) {
            ROS_printf(0, "HB_VOT_GetVideoLayerUpScale failed.\n");
        }
        stUpScale.src_width = nWidth;
        stUpScale.src_height = nHeight;
        stUpScale.tgt_width = 1920;
        stUpScale.tgt_height = 1080;
        ret = HB_VOT_SetVideoLayerUpScale(0, &stUpScale);
        if (ret) {
            ROS_printf(0, "HB_VOT_SetVideoLayerUpScale failed.\n");
        }
    }
    // 缩放 end
    ret = HB_VOT_EnableVideoLayer(0);
    if (ret) {
        ROS_printf(0, "HB_VOT_EnableVideoLayer failed.\n");
        HB_VOT_Disable(0);
        goto err;
    }

    ret = HB_VOT_SetChnAttr(0, 0, &vot_info->m_stChnAttr);
    if (ret) {
        ROS_printf(0, "HB_VOT_SetChnAttr 0: %d\n", ret);
        HB_VOT_DisableVideoLayer(0);
        HB_VOT_Disable(0);
        goto err;
    }

    ret = HB_VOT_SetChnCrop(0, 0, &vot_info->m_cropAttrs);
    ROS_printf(2, "HB_VOT_SetChnCrop: %d\n", ret);

    ret = HB_VOT_EnableChn(0, 0);
    if (ret) {
        ROS_printf(0, "HB_VOT_EnableChn: %d\n", ret);
        HB_VOT_DisableVideoLayer(0);
        HB_VOT_Disable(0);
        goto err;
    }

err:
    return ret;
}

int x3_vot_deinit(x3_vot_info_t* vot_info)
{
     int ret = 0;
    HB_VOT_SetVideoLayerAttr(0, &vot_info->m_stLayerAttr);
    ret = HB_VOT_DisableChn(0, 0);
    if (ret) {
        ROS_printf(0, "HB_VOT_DisableChn failed.\n");
    }

    ret = HB_VOT_DisableVideoLayer(0);
    if (ret) {
        ROS_printf(0, "HB_VOT_DisableVideoLayer failed.\n");
    }

    ret = HB_VOT_Disable(0);
    if (ret) {
        ROS_printf(0, "HB_VOT_Disable failed.\n");
    }
    ROS_printf(2, "x3_vot_deinit success.\n");
    return ret;
}
