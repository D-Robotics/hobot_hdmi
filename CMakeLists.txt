# Copyright (c) 2024，D-Robotics.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5)
project(hobot_hdmi)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ai_msgs REQUIRED)
find_package(hbm_img_msgs REQUIRED)
find_package(hobot_cv REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV 4 REQUIRED)

add_definitions(-DSHARED_MEM_ENABLED)

# x3|rdkultra|x86
set(PREFIX_PATH x3)
set(SYS_ROOT ${CMAKE_SYSROOT})
if(PLATFORM_X3)
  message("build platform X3")
  add_definitions(-DPLATFORM_X3)
  set(PREFIX_PATH x3)
elseif(PLATFORM_X5)
  message("build platform X5")
  add_definitions(-DPLATFORM_X5)
  set(PREFIX_PATH x5)
else()
  message("invalid platform, build platform X3 default")
  add_definitions(-DPLATFORM_X3)
  set(PREFIX_PATH x3)
endif()

include_directories(include
  include/${PREFIX_PATH}
  ${PROJECT_SOURCE_DIR}
  ${SYS_ROOT}/usr/include/opencv/
  ${SYS_ROOT}/usr/include/
  ${SYS_ROOT}/usr/hobot/include/
)

link_directories(
  ${SYS_ROOT}/usr/lib/hbmedia
  ${SYS_ROOT}/usr/lib/hbbpu
  ${SYS_ROOT}/usr/lib/
  ${SYS_ROOT}/usr/hobot/lib/
)

if(PLATFORM_X3)
  add_executable(hobot_hdmi
    src/example.cpp
    src/x3/image_display_node.cpp
    src/x3/x3_vio_vot.c
  )
  target_link_libraries(hobot_hdmi
    isp_algo z hbmedia iar isp ion vio tinyalsa
  )
elseif(PLATFORM_X5)
  add_executable(hobot_hdmi
    src/example.cpp
    src/plugin_framework.cpp
    src/image_util.cpp
    src/x5/display_utils.c
    src/x5/display_framework.cpp
    src/x5/image_display_node.cpp
    src/x5/common_utils.c
  )
  target_link_libraries(hobot_hdmi
    pthread cjson dl drm alog hbmem cam vpf 
  )
endif()

ament_target_dependencies(
  hobot_hdmi
  ai_msgs
  cv_bridge
  hbm_img_msgs
  hobot_cv
  rclcpp
  sensor_msgs
  OpenCV
)

# Install executables
install(
  TARGETS hobot_hdmi
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  ${PROJECT_SOURCE_DIR}/launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

install(DIRECTORY
  ${PROJECT_SOURCE_DIR}/config/
  DESTINATION lib/${PROJECT_NAME}/config/
)

ament_package()
