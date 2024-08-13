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

#include "hobot_cv/hobotcv_imgproc.h"

#include "image_util.h"

// 使用hobotcv resize nv12格式图片，固定图片宽高比
int ResizeNV12Img(const char *in_img_data,
                  const int &in_img_height,
                  const int &in_img_width,
                  const int &resized_img_height,
                  const int &resized_img_width,
                  float &ratio_h,
                  float &ratio_w,
                  cv::Mat &out_img) {
  cv::Mat src(
      in_img_height * 3 / 2, in_img_width, CV_8UC1, (void *)(in_img_data));
  
  //高度向下取偶数
  int resized_height =
      resized_img_height % 2 == 0 ? resized_img_height : resized_img_height - 1;
  int resized_width =
      resized_img_width % 2 == 0 ? resized_img_width : resized_img_width - 1;

  ratio_w =
      static_cast<float>(in_img_width) / static_cast<float>(resized_img_width);
  ratio_h =
      static_cast<float>(in_img_height) / static_cast<float>(resized_img_height);

  return hobot_cv::hobotcv_resize(
      src, in_img_height, in_img_width, out_img, resized_height, resized_width);
}

int32_t BGRToNv12(cv::Mat &bgr_mat, cv::Mat &img_nv12) {
  auto height = bgr_mat.rows;
  auto width = bgr_mat.cols;

  if (height % 2 || width % 2) {
    std::cerr << "input img height and width must aligned by 2!";
    return -1;
  }
  cv::Mat yuv_mat;
  cv::cvtColor(bgr_mat, yuv_mat, cv::COLOR_BGR2YUV_I420);
  if (yuv_mat.data == nullptr) {
    std::cerr << "yuv_mat.data is null pointer" << std::endl;
    return -1;
  }

  auto *yuv = yuv_mat.ptr<uint8_t>();
  if (yuv == nullptr) {
    std::cerr << "yuv is null pointer" << std::endl;
    return -1;
  }
  img_nv12 = cv::Mat(height * 3 / 2, width, CV_8UC1);
  auto *ynv12 = img_nv12.ptr<uint8_t>();

  int32_t uv_height = height / 2;
  int32_t uv_width = width / 2;

  // copy y data
  int32_t y_size = height * width;
  memcpy(ynv12, yuv, y_size);

  // copy uv data
  uint8_t *nv12 = ynv12 + y_size;
  uint8_t *u_data = yuv + y_size;
  uint8_t *v_data = u_data + uv_height * uv_width;

  for (int32_t i = 0; i < uv_width * uv_height; i++) {
    *nv12++ = *u_data++;
    *nv12++ = *v_data++;
  }
  return 0;
}

int32_t Nv12ToBGR(const char *in_img_data, const int &in_img_height, const int &in_img_width, cv::Mat &bgr_mat) {
  cv::Mat Nv12_image(in_img_height + in_img_height / 2, in_img_width, CV_8UC1, (void*)in_img_data);

  cv::cvtColor(Nv12_image, bgr_mat, cv::COLOR_YUV2BGR_NV12);
  return 0;
}