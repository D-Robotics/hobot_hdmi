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

#ifndef IMAGE_UTIL_H_
#define IMAGE_UTIL_H_

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

template<typename T>
int readbinary(const std::string &filename, T* &dataOut) {
  std::ifstream ifs(filename, std::ios::in | std::ios::binary);
  if (!ifs) {
    return -1;
  }
  ifs.seekg(0, std::ios::end);
  int len = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  char* data = new char[len];
  ifs.read(data, len);
  dataOut = reinterpret_cast<T *>(data);
  return len / sizeof(T);
}

int ResizeNV12Img(const char *in_img_data,
                  const int &in_img_height,
                  const int &in_img_width,
                  const int &resized_img_height,
                  const int &resized_img_width,
                  float &ratio_h,
                  float &ratio_w,
                  cv::Mat &out_img);

int32_t BGRToNv12(cv::Mat &bgr_mat, cv::Mat &img_nv12);

int32_t Nv12ToBGR(const char *in_img_data, const int &in_img_height, const int &in_img_width, cv::Mat &bgr_mat);

#endif  // IMAGE_UTIL_H_