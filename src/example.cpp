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

#include <string>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "include/image_display.h"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  RCLCPP_WARN(rclcpp::get_logger("example"), "This is image_display example!");

  rclcpp::NodeOptions opt;
  auto node = std::make_shared<ImageDisplay>(opt);
  RCLCPP_WARN(rclcpp::get_logger("example"), "image_display init!");

  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(node);
  RCLCPP_WARN(rclcpp::get_logger("example"), "image_display add_node!");
  exec.spin();

  rclcpp::shutdown();
  return 0;
}
