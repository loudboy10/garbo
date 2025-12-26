#This is currently the main launch file for simulating the robot in Gazebo


import os

from ament_index_python.packages import get_package_share_directory


from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node


def generate_launch_description():

    # Include the robot_state_publisher launch file, provided by our own package. Force sim time to be enabled
    # !!! MAKE SURE YOU SET THE PACKAGE NAME CORRECTLY !!!

    package_name = 'garbo'  # <--- CHANGE ME

    rsp = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory(package_name), 'launch', 'rsp.launch.py'
        )]), launch_arguments={'use_sim_time': 'true'}.items()
    )

    world = LaunchConfiguration('world')

    world_arg = DeclareLaunchArgument(
        'world',
        default_value="empty.sdf",
        description='World to load'
    )

    #joystick = IncludeLaunchDescription(
    #             PythonLaunchDescriptionSource([os.path.join(
    #                get_package_share_directory(package_name),'launch','joystick.launch.py'
    #            )]), launch_arguments={'use_sim_time': 'true'}.items()
    #)

    twist_mux_params = os.path.join(get_package_share_directory(package_name),'config','twist_mux_topics.yaml')
    twist_mux = Node(
            package="twist_mux",
            executable="twist_mux",
            parameters=[twist_mux_params, {'use_sim_time': True}, {'use_stamped': True}],
            #remappings=[('/cmd_vel_out','/diff_cont/cmd_vel_unstamped') ],
            remappings=[('/cmd_vel_out', '/cmd_vel') ],

        )


    # Include the Gazebo launch file, provided by the ros_gz_sim package
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory('ros_gz_sim'), 'launch', 'gz_sim.launch.py')]),
        launch_arguments={'gz_args': ['-r -v4 ', world], 'on_exit_shutdown': 'true'}.items()
    )

    # Run the spawner node from the ros_gz_sim package. The entity name
    # doesn't really matter if you only have a single robot.
    spawn_entity = Node(package='ros_gz_sim', executable='create',
                        arguments=['-topic', 'robot_description',
                                   '-name', 'garbo',
                                   '-z', '0.33'],  # spawn a bit above the ground to prevent clipping. The bot will fall into place.
                        output='screen')

    # Launch the ROS-Gazebo bridge for normal topics
    bridge_params = os.path.join(
        get_package_share_directory(package_name), 'config', 'gz_bridge.yaml')
    ros_gz_bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        arguments=[
            '--ros-args',
            '-p',
            f'config_file:={bridge_params}',
        ]
    )

    # ROS-Gazebo image bridge
    ros_gz_image_bridge = Node(
        package="ros_gz_image",
        executable="image_bridge",
        arguments=["/camera/image_raw"]
    )

    diff_drive_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=[
            "diff_cont",
            '--controller-ros-args',
            '-r /diff_cont/cmd_vel:=/cmd_vel'
        ],
    )

    joint_broad_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_broad"],
    )
    
    ekf_params = os.path.join(get_package_share_directory(package_name),'config','ekf.yaml')
    robot_localization_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[ekf_params, {'use_sim_time': LaunchConfiguration('use_sim_time')}]
)
    

    # Launch them all!
    return LaunchDescription([
        rsp,
        world_arg,
        #joystick,
        twist_mux,      
        gazebo,
        spawn_entity,
        robot_localization_node,
        ros_gz_bridge,
        ros_gz_image_bridge,
        diff_drive_spawner,
        joint_broad_spawner
    ])
