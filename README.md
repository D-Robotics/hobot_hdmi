English| [简体中文](./README_cn.md)

# Function Introduction

The hobot_hdmi package is used to display image messages published by a ROS2 Node through HDMI. It supports ROS standard format and also supports subscribing via shared memory.

# Compilation

## Dependency Libraries

- sensor_msgs
- hbm_img_msgs

hbm_img_msgs is a custom message format used for publishing shared memory type image data, defined in hobot_msgs.

## Development Environment

- Programming Language: C/C++
- Development Platform: X3/X86
- System Version: Ubuntu 20.0.4
- Compilation Toolchain: Linux GCC 9.3.0/Linaro GCC 9.3.0

## Compilation

Supports compilation on X3 Ubuntu system and cross-compilation using docker on a PC.

### Compilation on Ubuntu System

1. Compilation Environment Confirmation
   - X3 Ubuntu system is installed on the board.
   - Current compilation terminal has set the TogetherROS environment variable: `source PATH/setup.bash`. Where PATH is the installation path of TogetherROS.
   - ROS2 compilation tool colcon is installed, installation command: `pip install -U colcon-common-extensions`

2. Compilation

Compilation command: `colcon build --packages-select hobot_hdmi`

### Docker Cross-Compilation

1. Compilation Environment Confirmation

   - Compilation in docker, and TogetherROS is already installed in docker. Docker installation, cross-compilation instructions, TogetherROS compilation, and deployment instructions can be found in the robot development platform robot_dev_config repo's README.md.

2. Compilation

   - Compilation command:

```
export TARGET_ARCH=aarch64
export TARGET_TRIPLE=aarch64-linux-gnu
export CROSS_COMPILE=/usr/bin/$TARGET_TRIPLE-

colcon build --packages-select hobot_hdmi \
   --merge-install \
   --cmake-force-configure \
   --cmake-args \
   --no-warn-unused-cli \
   -DCMAKE_TOOLCHAIN_FILE=`pwd`/robot_dev_config/aarch64_toolchainfile.cmake
```

## Notes

1. hbm_img_msgs package has been compiled.


# Instructions

## Dependencies

## Parameters

| Parameter   | Meaning              | Value                         | Default               |
| ----------- | -------------------- | ----------------------------- | --------------------- |
| sub_img_topic   | Subscribed Image Topic | String                    |      image_raw       |
| io_method   | Data Transfer Method    | String, supports only "ros/shared_mem" |      ros          |


## Execution

After successful compilation, copy the generated install path to the Horizon X3 development board (if compiling on X3, ignore the copying step), and run the following command:

### **Ubuntu**

To start using ros2 run:

```
export COLCON_CURRENT_PREFIX=./install
source ./install/setup.bash

# Publish image data
ros2 run mipi_cam mipi_cam --ros-args -p io_method:=shared_mem -p out_format:=nv12

# Specify topic as hbmem_img, receive data published by the publishing end through share mem pub:
ros2 run hobot_hdmi hobot_hdmi --ros-args -p sub_img_topic:=/hbmem_img -p io_method:=shared_mem

```

To start using launch file:

```
export COLCON_CURRENT_PREFIX=./install
source ./install/setup.bash

# Launch file for starting
ros2 launch install/share/hobot_hdmi/launch/hobot_hdm.launch.py

```

### **Linux**

```
export ROS_LOG_DIR=/userdata/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./install/lib/

# Publish image data
/userdata/install/lib/mipi_cam/mipi_cam --ros-args -p io_method:=shared_mem

# Specify topic as hbmem_img, receive data published by share mem pub
/userdata/install/lib/hobot_hdmi/hobot_hdmi --ros-args -p sub_img_topic:=hbmem_img -p io_method:=shared_mem

```

## Notice


# Result Analysis

## X3 Result Display

```
root@ubuntu:/userdata# ros2 run hobot_hdmi hobot_hdm --ros-args -p sub_img_topic:=/hbmem_img -p io_method:=shared_mem
[WARN] [1659415693.142894609] [example]: This is image_display example!
[WARN] [1659415693.210313621] [hobot_hdmi]: Create topic: /hbmem_img,io=shared_mem.
[WARN] [1659415693.212576172] [hobot_hdmi]: Create hbmem_subscription with topic_name: /hbmem_img, sub = 0x7fce131570
[WARN] [1659415693.212699293] [example]: image_display init!
[WARN] [1659415693.213532391] [example]: image_display add_node!
[INFO] [1659415693.243422334] [hobot_hdm]: stLayer width:1920

[INFO] [1659415693.243543913] [hobot_hdm]: stLayer height:1080

[INFO] [1659415693.243706825] [hobot_hdm]: HB_VOT_SetChnCrop: 0
```

The log above shows that the HDMI output resolution is 1920*1080

## Web Display Effects


# Frequently Asked Questions
