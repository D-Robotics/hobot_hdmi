[English](./README.md) | 简体中文

# 功能介绍

hobot_hdmi package用于通过 HDMI 显示接收 ROS2 Node 发布的image msg。支持ROS标准格式，也支持 share mem 方式订阅。



# 编译

## 依赖库

- sensor_msgs
- hbm_img_msgs
- cv_bridge
- hobot_cv

hbm_img_msgs为自定义消息格式，用于发布shared memory类型图像数据，定义在hobot_msgs中。

## 开发环境

- 编程语言: C/C++
- 开发平台: X3/X5/X86
- 系统版本：Ubuntu 20.04/Ubuntu 22.04
- 编译工具链:Linux GCC 9.3.0/Linaro GCC 11.4.0

## 编译

 支持在X3/X5 Ubuntu系统上编译和在PC上使用docker交叉编译两种方式。

### Ubuntu板端编译

1. 编译环境确认 
   - 板端已安装X3/X5 Ubuntu系统。
   - 当前编译终端已设置TogetherROS环境变量：`source PATH/setup.bash`。其中PATH为TogetherROS的安装路径。
   - 已安装ROS2编译工具colcon，安装命令：`pip install -U colcon-common-extensions`
2. 编译

编译命令：`colcon build --packages-select hobot_hdmi`

### Docker交叉编译

1. 编译环境确认

   - 在docker中编译，并且docker中已经安装好TogetherROS。docker安装、交叉编译说明、TogetherROS编译和部署说明详见机器人开发平台robot_dev_config repo中的README.md。

2. 编译

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

## 注意事项

1、已编译hbm_img_msgs package

2、使用X5 hdmi display 功能时, 需要进行如下操作

```shell
modprobe panel-jc-050hd134
modprobe vio_n2d
modprobe lontium_lt8618
modprobe vs-x5-syscon-bridge
modprobe vs_drm

cp -r install/lib/hobot_hdmi/config .
```

# 使用介绍

## 依赖

## 参数

| 参数名      | 适用平台 | 含义                 | 取值                          | 默认值                |
| ----------- | ---- | -------------------- | ----------------------------- | --------------------- |
| only_show_image   | X5 | 是否只展示图像      | true: 只展示图像, false: 展示图像加渲染结果(X3支持 true 模式)  |      true       |
| ai_msg_sub_topic_name   | X5 | 订阅ai结果话题, 仅当 only_show_image 为 false有效   | 字符串                         |      /hobot_detection       |
| ros_img_sub_topic_name   | X3, X5 | 订阅Ros图片话题      | 字符串                         |      /image       |
| is_shared_mem   | X3, X5 | 传输数据方式          | true: 零拷贝, false: Ros话题    |      false          |


## 运行

编译成功后，将生成的install路径拷贝到地平线X3开发板上（如果是在X3上编译，忽略拷贝步骤），并执行如下命令运行：

### **Ubuntu**

运行方式1，使用ros2 run启动：

```
export COLCON_CURRENT_PREFIX=./install
source ./install/setup.bash

# 发布图片数据
ros2 run mipi_cam mipi_cam --ros-args -p io_method:=shared_mem -p out_format:=nv12

# 指明topic 为 hbmem_img，接收 发布端通过share mem pub 的数据：
ros2 run hobot_hdmi hobot_hdmi --ros-args -p is_shared_mem:=true

```
运行方式2，使用launch文件启动：
```
export COLCON_CURRENT_PREFIX=./install
source ./install/setup.bash

# 启动launch文件
ros2 launch install/share/hobot_hdmi/launch/hobot_hdmi.launch.py

```

### **Linux**

```
export ROS_LOG_DIR=/userdata/
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./install/lib/

# 发布图片数据
/userdata/install/lib/mipi_cam/mipi_cam --ros-args -p io_method:=shared_mem

# 指明topic 为 hbmem_img，接收 发布端通过share mem pub 的数据
/userdata/install/lib/hobot_hdmi/hobot_hdmi --ros-args -p is_shared_mem:=true

```

## 注意事项


# 结果分析

## X3结果展示

```
root@ubuntu:/userdata# ros2 run hobot_hdmi hobot_hdmi --ros-args -p sub_img_topic:=/hbmem_img -p io_method:=shared_mem
[WARN] [1659415693.142894609] [example]: This is image_display example!
[WARN] [1659415693.210313621] [hobot_hdmi]: Create topic: /hbmem_img,io=shared_mem.
[WARN] [1659415693.212576172] [hobot_hdmi]: Create hbmem_subscription with topic_name: /hbmem_img, sub = 0x7fce131570
[WARN] [1659415693.212699293] [example]: image_display init!
[WARN] [1659415693.213532391] [example]: image_display add_node!
[INFO] [1659415693.243422334] [hobot_hdmi]: stLayer width:1920

[INFO] [1659415693.243543913] [hobot_hdmi]: stLayer height:1080

[INFO] [1659415693.243706825] [hobot_hdmi]: HB_VOT_SetChnCrop: 0
```

以上log显示，hdmi输出分辨率为1920*1080

# 常见问题
