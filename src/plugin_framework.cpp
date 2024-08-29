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

#include "plugin_framework.h"

bool CheckScale(const ai_msgs::msg::PerceptionTargets::SharedPtr &ai_msg) {
  for (size_t idx = 0; idx < ai_msg->targets.size(); idx++) {
    const auto &target = ai_msg->targets.at(idx);
    if (target.rois[0].type == "body") {
      return true;
    }
  }
  return false;
}

int32_t SegPlugin::RenderSeg(
    cv::Mat &mat,
    const ai_msgs::msg::PerceptionTargets::SharedPtr &ai_msg) {
  if (!ai_msg) return -1;

  for (size_t idx = 0; idx < ai_msg->targets.size(); idx++) {
    const auto &target = ai_msg->targets.at(idx);

    if (!target.captures.empty()) {
      for (auto capture: target.captures) {
        int parsing_width = capture.img.width;
        int parsing_height = capture.img.height;
        cv::Mat parsing_img(parsing_height, parsing_width, CV_8UC4);
        uint8_t *parsing_img_ptr = parsing_img.ptr<uint8_t>();

        for (int h = 0; h < parsing_height; ++h) {
          for (int w = 0; w < parsing_width; ++w) {
            auto id = static_cast<size_t>(capture.features[h * parsing_width + w]);
            id = id >= 80? (id % 80) + 1: id;
            *parsing_img_ptr++ = 255;
            *parsing_img_ptr++ = bgr_putpalette[id * 3];
            *parsing_img_ptr++ = bgr_putpalette[id * 3 + 1];
            *parsing_img_ptr++ = bgr_putpalette[id * 3 + 2];
          }
        }

        cv::resize(parsing_img, parsing_img, mat.size(), 0, 0);
        mat = std::move(parsing_img);
      }
    }
  }
  return 0;
}

int32_t AttributesPlugin::RenderAttributes(cv::Mat &mat,
    const std::vector<ai_msgs::msg::Attribute> &attributes,
    const sensor_msgs::msg::RegionOfInterest &rect,
    const std::string &type,
    const uint64 &track_id) {

  std::vector<std::string> words;
  std::string word = type + ": " + std::to_string(track_id);
  words.push_back(word);
  int max_size = word.size();
  for (auto &attribute: attributes) {
    word = attribute.type;
    word += ": ";
    if (attribute.type == "gesture") {
      word += gesture_map.at(attribute.value);
    } else {
      word += std::to_string(attribute.value);
    }
    max_size = word.size() > max_size ? word.size() : max_size;
    words.push_back(word);
  }

  int box_width = max_size * x_stride;
  int box_height = (1 + attributes.size()) * y_stride;

  int index_x = rect.x_offset;
  int index_y = rect.y_offset - 10;

  index_x = index_x > 0 ? index_x : 0;
  index_x = index_x < (img_width - box_width) ? index_x : (img_width - box_width);
  index_y = index_y > y_stride ? index_y : y_stride;
  index_y = index_y < (img_height - box_height) ? index_y : (img_height - box_height);

  cv::rectangle(mat,
        cv::Point(index_x, index_y - y_stride - 10),
        cv::Point(index_x + box_width,
                  index_y + box_height),
        cv::Scalar(255, 87, 75, 61),
        -1);

  cv::line(mat,
        cv::Point(index_x + 10, index_y + 5),
        cv::Point(index_x + box_width - 10, index_y + 5),
        cv::Scalar(255, 255, 255, 255),
        1);

  for (const std::string &word: words) {
    cv::putText(mat,
            word,
            cv::Point2f(index_x, index_y),
            cv::HersheyFonts::FONT_HERSHEY_SIMPLEX,
            1.0,
            cv::Scalar(255, 255, 255, 255),
            2.0);
    index_y += y_stride;
  }

  return 0;
}

int32_t DetPlugin::RenderDet(
    cv::Mat &mat,
    const ai_msgs::msg::PerceptionTargets::SharedPtr &ai_msg,
    float ratio_h,
    float ratio_w) {
  if (!ai_msg) return -1;

  bool is_scale = CheckScale(ai_msg);
  if (!is_scale) {
    ratio_h = 1.0;
    ratio_w = 1.0;
  }
  for (size_t idx = 0; idx < ai_msg->targets.size(); idx++) {
    const auto &target = ai_msg->targets.at(idx);
    const auto roi = target.rois[0];

    sensor_msgs::msg::RegionOfInterest rect;
    rect.x_offset = roi.rect.x_offset / ratio_w;
    rect.y_offset = roi.rect.y_offset / ratio_h;
    rect.width = roi.rect.width / ratio_w;
    rect.height = roi.rect.height / ratio_h;

    cv::rectangle(mat,
                  cv::Point(rect.x_offset, rect.y_offset),
                  cv::Point(rect.x_offset + rect.width, rect.y_offset + rect.height),
                  box_color,
                  3);
    std::string roi_type = target.type;
    if (!roi.type.empty()) {
      roi_type = roi.type;
    }
    auto attributes_plugin = std::make_shared<AttributesPlugin>(1080, 1920);
    attributes_plugin->RenderAttributes(mat, target.attributes, rect, roi_type, target.track_id);
  }
  return 0;
}

int32_t RenderLine(cv::Mat &mat,
                    const std::vector<geometry_msgs::msg::Point32> &point,
                    int start_id, int end_id,
                    float ratio_w, float ratio_h,
                    cv::Scalar color) {
  cv::line(mat, 
          cv::Point(point[start_id].x / ratio_w, point[start_id].y / ratio_h),
          cv::Point(point[end_id].x / ratio_w, point[end_id].y / ratio_h),
          color, 3);
  return 0;
}

int32_t KPSPlugin::RenderKPS(
    cv::Mat &mat,
    const ai_msgs::msg::PerceptionTargets::SharedPtr &ai_msg,
    const float ratio_h,
    const float ratio_w) {
  if (!ai_msg) return -1;

  for (size_t idx = 0; idx < ai_msg->targets.size(); idx++) {
    const auto &target = ai_msg->targets.at(idx);

    for (const auto &lmk : target.points) {

      if (lmk.type == "body_kps") {
        // head
        RenderLine(mat, lmk.point, 0, 1, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 0, 2, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 1, 2, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 1, 3, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 2, 4, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 3, 5, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 4, 6, ratio_w, ratio_h, kps_colors[3]);
        
        // body
        RenderLine(mat, lmk.point, 5, 6, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 6, 12, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 5, 11, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 11, 12, ratio_w, ratio_h, kps_colors[0]);

        // hand
        RenderLine(mat, lmk.point, 6, 8, ratio_w, ratio_h, kps_colors[1]);
        RenderLine(mat, lmk.point, 8, 10, ratio_w, ratio_h, kps_colors[2]);
        
        RenderLine(mat, lmk.point, 5, 7, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 7, 9, ratio_w, ratio_h, kps_colors[0]);

        // leg
        RenderLine(mat, lmk.point, 12, 14, ratio_w, ratio_h, kps_colors[1]);
        RenderLine(mat, lmk.point, 14, 16, ratio_w, ratio_h, kps_colors[2]);
        RenderLine(mat, lmk.point, 11, 13, ratio_w, ratio_h, kps_colors[0]);
        RenderLine(mat, lmk.point, 13, 15, ratio_w, ratio_h, kps_colors[0]);

        for (const auto &pt : lmk.point) {
          cv::circle(mat, cv::Point(pt.x / ratio_w, pt.y / ratio_h), 8, kps_colors[0], 3);
          cv::circle(mat, cv::Point(pt.x / ratio_w, pt.y / ratio_h), 8 - 4, cv::Scalar(255, 255, 255, 255), cv::FILLED);
        }
      } else if (lmk.type == "hand_kps") {
        for (const auto &pt : lmk.point) {
          cv::circle(mat, cv::Point(pt.x / ratio_w, pt.y / ratio_h), 8, kps_colors[0], -1);
        }

        // Thumb
        RenderLine(mat, lmk.point, 0, 1, ratio_w, ratio_h, kps_colors[1]);
        RenderLine(mat, lmk.point, 1, 2, ratio_w, ratio_h, kps_colors[1]);
        RenderLine(mat, lmk.point, 2, 3, ratio_w, ratio_h, kps_colors[1]);
        RenderLine(mat, lmk.point, 3, 4, ratio_w, ratio_h, kps_colors[1]);

        // Index finger
        RenderLine(mat, lmk.point, 0, 5, ratio_w, ratio_h, kps_colors[6]);
        RenderLine(mat, lmk.point, 5, 6, ratio_w, ratio_h, kps_colors[6]);
        RenderLine(mat, lmk.point, 6, 7, ratio_w, ratio_h, kps_colors[6]);
        RenderLine(mat, lmk.point, 7, 8, ratio_w, ratio_h, kps_colors[6]);

        // Middle finger
        RenderLine(mat, lmk.point, 1, 9, ratio_w, ratio_h, kps_colors[5]);
        RenderLine(mat, lmk.point, 9, 10, ratio_w, ratio_h, kps_colors[5]);
        RenderLine(mat, lmk.point, 10, 11, ratio_w, ratio_h, kps_colors[5]);
        RenderLine(mat, lmk.point, 11, 12, ratio_w, ratio_h, kps_colors[5]);

        // Ring finger
        RenderLine(mat, lmk.point, 1, 13, ratio_w, ratio_h, kps_colors[7]);
        RenderLine(mat, lmk.point, 13, 14, ratio_w, ratio_h, kps_colors[7]);
        RenderLine(mat, lmk.point, 14, 15, ratio_w, ratio_h, kps_colors[7]);
        RenderLine(mat, lmk.point, 15, 16, ratio_w, ratio_h, kps_colors[7]);

        // Little finger
        RenderLine(mat, lmk.point, 1, 17, ratio_w, ratio_h, kps_colors[3]);
        RenderLine(mat, lmk.point, 17, 18, ratio_w, ratio_h, kps_colors[3]);
        RenderLine(mat, lmk.point, 18, 19, ratio_w, ratio_h, kps_colors[3]);
        RenderLine(mat, lmk.point, 19, 20, ratio_w, ratio_h, kps_colors[3]);
      }
    }
  }
  return 0;
}