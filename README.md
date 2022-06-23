Getting Started with hobot_hdmi
=======

# Intro

hobot_hdmi package用于通过 HDMI 显示接收 ROS2 Node 发布的image msg。支持ROS标准格式，也支持 share mem 方式订阅。

# Build
---
## Dependency

ros package：
- sensor_msgs
- hbm_img_msgs

hbm_img_msgs为自定义消息格式，用于发布shared memory类型图像数据，定义在hobot_msgs中。

## 开发环境

- 编程语言: C/C++
- 开发平台: X3/X86
- 系统版本：Ubuntu 20.0.4
- 编译工具链:Linux GCC 9.3.0/Linaro GCC 9.3.0

## 编译

 支持在X3 Ubuntu系统上编译和在PC上使用docker交叉编译两种方式。

### 编译选项

SHARED_MEM

- shared mem（共享内存传输）使能开关，默认关闭（ON），编译时使用-DBUILD_HBMEM=OFF命令打开。
- 如果打开，编译和运行会依赖hbm_img_msgs pkg，并且需要使用tros进行编译。
- 如果关闭，编译和运行不依赖hbm_img_msgs pkg，支持使用原生ros和tros进行编译。

### X3 Ubuntu系统上编译
1、编译环境确认

- 当前编译终端已设置ROS环境变量：`source /opt/ros/foxy/setup.bash`。
- 已安装ROS2编译工具colcon。安装的ROS不包含编译工具colcon，需要手动安装colcon。colcon安装命令：`apt update; apt install python3-colcon-common-extensions`
- 已依赖pkg ，详见 Dependency 部分

2、编译：
  - 订阅share mem 方式发布的图片：`colcon build --packages-select hobot_hdmi`
  这个需要先配置 TROS 环境，例如：`source /opt/tros/setup.bash`
  - 支持订阅ROS2标准格式图片：`colcon build --packages-select hobot_hdmi --cmake-args -DBUILD_HBMEM=OFF`。

### docker交叉编译

1、编译环境确认

- 在docker中编译，并且docker中已经安装好tros。docker安装、交叉编译说明、tros编译和部署说明：http://gitlab.hobot.cc/robot_dev_platform/robot_dev_config/blob/dev/README.md
- 已编译hbm_img_msgs package

2、编译

- 编译命令： 

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
- 其中SYS_ROOT为交叉编译系统依赖路径，此路径具体地址详见第1步“编译环境确认”的交叉编译说明。

# Usage

## 目前参数列表：

| 参数名      | 含义                 | 取值                          | 默认值                |
| ----------- | -------------------- | ----------------------------- | --------------------- |
| sub_img_topic   | 订阅图片主题      | 字符串                         |      image_raw       |
| io_method   | 传输数据方式          | 字符串，只支持 "ros/shared_mem"    |      ros          |

## X3 Ubuntu系统
编译成功后，将生成的install路径拷贝到地平线X3开发板上（如果是在X3上编译，忽略拷贝步骤），并执行如下命令运行

```
export COLCON_CURRENT_PREFIX=./install
source ./install/local_setup.sh
# 发布图片数据
ros2 run mipi_cam mipi_cam --ros-args -p io_method:=shared_mem -p out_format:=nv12
# 指明topic 为 hbmem_img，接收 发布端通过share mem pub 的数据：
ros2 run hobot_hdmi hobot_hdmi --ros-args -p sub_img_topic:=/hbmem_img -p io_method:=shared_mem
```

## X3 linaro系统

把在docker 交叉编译的install 目录拷贝到linaro 系统下，例如:/userdata
需要首先指定依赖库的路径，例如：
`export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/userdata/install/lib`

修改 ROS_LOG_DIR 的路径，否则会创建在 /home 目录下，需要执行 mount -o remount,rw /，才可以

运行 hobot_hdmi
```
#/userdata/install/lib/mipi_cam/mipi_cam --ros-args -p io_method:=shared_mem
# 传参方式
#/userdata/install/lib/hobot_hdmi/hobot_hdmi --ros-args -p sub_img_topic:=hbmem_img -p io_method:=shared_mem

```