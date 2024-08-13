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

import os

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from ament_index_python import get_package_share_directory

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'display_ros_img_sub_topic_name',
            default_value='/image',
            description='The topic name of image message'),
        DeclareLaunchArgument(
            'display_is_shared_mem',
            default_value='True',
            description='using zero copy or not'),
        DeclareLaunchArgument(
            'display_only_show_image',
            default_value='False',
            description='only show image'),
        DeclareLaunchArgument(
            'display_log_level',
            default_value='warn',
            description='The log level of Node'),
        # 启动图片发布pkg，output_image_w与output_image_h设置为0代表不改变图片的分辨率
        Node(
            package='hobot_hdmi',
            executable='hobot_hdmi',
            output='screen',
            parameters=[
                {"ros_img_sub_topic_name": LaunchConfiguration('display_ros_img_sub_topic_name')},
                {"is_shared_mem": LaunchConfiguration('display_is_shared_mem')},
                {"only_show_image": LaunchConfiguration('display_only_show_image')}
            ],
            arguments=['--ros-args', '--log-level', LaunchConfiguration('display_log_level')]
        )
    ])