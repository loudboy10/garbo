# This is used to open a previously saved
# .yaml/.pgm format map.

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    map_file_path = os.path.join(
        get_package_share_directory('garbo'),  # Replace with your package name
        'worlds',
        'driveway_edited.yaml'  # Replace with your map file name
    )

    return LaunchDescription([
        Node(
            package='nav2_map_server',
            executable='map_server',
            name='map_server',
            output='screen',
            parameters=[{'yaml_filename': map_file_path}]
        ),
        # ... other nodes for your robot, SLAM Toolbox, etc.
    ])
