import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, LaunchConfiguration
from launch_ros.actions import Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from ros_gz_bridge.actions import RosGzBridge
from ros_gz_sim.actions import GzServer


def generate_launch_description():
    
    pkg_share = get_package_share_directory('garbo')
    ros_gz_sim_share = get_package_share_directory('ros_gz_sim')
    gz_spawn_model_launch_source = os.path.join(ros_gz_sim_share, "launch", "gz_spawn_model.launch.py")
    default_model_path = os.path.join(pkg_share, 'description', 'garbo_description.sdf')
    default_rviz_config_path = os.path.join(pkg_share, 'rviz', 'config.rviz')
    world_path = os.path.join(pkg_share, 'world', 'driveway.sdf')
    bridge_config_path = os.path.join(pkg_share, 'config', 'bridge_config.yaml')
    ekf_config_path = os.path.join(pkg_share, 'config', 'ekf.yaml')
    twist_mux_config_path = os.path.join(pkg_share, 'config', 'twist_mux.yaml')
    apriltag_config_path = os.path.join(pkg_share, 'config', 'apriltag.yaml')



#https://docs.ros.org/en/jazzy/p/image_proc/doc/tutorials.html#launch-image-proc-components
    composable_nodes = [
        ComposableNode(
            package='image_proc',
            plugin='image_proc::RectifyNode',
            name='rectify_node',
            remappings=[
                ('image', 'depth_camera/image_raw'),
                ('image_rect', 'image_rect')
            ],
        ),
        ComposableNode(
            package='image_proc',
            plugin='image_proc::CropDecimateNode',
            name='crop_decimate_node',
            remappings=[
                ('in/image_raw', 'image_rect'),
                ('out/image_raw', 'image_downsized')
            ],
            parameters=[{
                'decimation_x': 2, 'decimation_y': 2,
            }]
        )
    ]

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': Command(['xacro ', LaunchConfiguration('model')])}, {'use_sim_time': LaunchConfiguration('use_sim_time')}]
    )
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', LaunchConfiguration('rvizconfig')],
    )
    gz_server = GzServer(
        world_sdf_file=world_path,
        container_name='ros_gz_container',
        create_own_container='True',
        use_composition='True',
    )
    ros_gz_bridge = RosGzBridge(
        bridge_name='ros_gz_bridge',
        config_file=bridge_config_path,
        container_name='ros_gz_container',
        create_own_container='False',
        use_composition='True',
    )
    depth_camera_bridge_image = Node(
        package='ros_gz_image',
        executable='image_bridge',
        name='bridge_gz_ros_depth_camera_image',
        output='screen',
        parameters=[{'use_sim_time': True}],
        arguments=['/depth_camera/image'],
    )
    depth_camera_bridge_depth = Node(
        package='ros_gz_image',
        executable='image_bridge',
        name='bridge_gz_ros_depth_camera_depth',
        output='screen',
        parameters=[{'use_sim_time': True}],
        arguments=['/depth_camera/depth_image'],
    )
    rear_camera_bridge_image = Node(
        package='ros_gz_image',
        executable='image_bridge',
        name='bridge_gz_ros_rear_camera_image',
        output='screen',
        parameters=[{'use_sim_time': True}],
        arguments=['/rear_camera/image_raw'],
    )
    spawn_entity = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(gz_spawn_model_launch_source),
        launch_arguments={
            'world': 'driveway',
            'topic': '/robot_description',
            'entity_name': 'garbo',
            'z': '0.65',
        }.items(),
    )
    robot_localization_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_node',
        output='screen',
        parameters=[ekf_config_path, {'use_sim_time': LaunchConfiguration('use_sim_time')}],
    )
    twist_mux = Node(
        package="twist_mux",
        executable="twist_mux",
        parameters=[twist_mux_config_path, {'use_sim_time': True}, {'use_stamped': True}],
        remappings=[('/cmd_vel_out', '/demo/cmd_vel') ],
    )
    apriltag_node = Node(
        package='apriltag_ros', # The name of the package containing the node executable
        executable='apriltag_node', # The name of the executable for the AprilTag detector
        name='apriltag_detector',
        output='screen',
        # Optional: Remap topics if necessary to match your system (e.g., camera input)
        remappings=[ 
            #('/image_rect', '/image_downsized'),
            #('/image_rect', '/depth_camera/image_raw'),
            ('/camera_info', '/depth_camera/camera_info')
        ],
        parameters=[apriltag_config_path, {'use_sim_time': True}],
    )   
    container = ComposableNodeContainer(
        name='image_proc_container',
        namespace='',
        package='rclcpp_components',
        executable='component_container',
        composable_node_descriptions=composable_nodes,
    )



    return LaunchDescription([
        DeclareLaunchArgument(name='model', default_value=default_model_path, description='Absolute path to robot model file'),
        DeclareLaunchArgument(name='rvizconfig', default_value=default_rviz_config_path, description='Absolute path to rviz config file'),
        DeclareLaunchArgument(name='use_sim_time', default_value='True', description='Flag to enable use_sim_time'),
        ExecuteProcess(cmd=['gz', 'sim', '-g'], output='screen'),
        robot_state_publisher_node,
        rviz_node,
        gz_server,
        ros_gz_bridge,
        depth_camera_bridge_image,
        depth_camera_bridge_depth,
        rear_camera_bridge_image,
        spawn_entity,
        robot_localization_node,
        twist_mux,
        apriltag_node,
        container,
    ])