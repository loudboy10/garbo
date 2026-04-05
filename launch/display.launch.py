import os

from ament_index_python.packages import get_package_share_directory
from launch.actions import(
  DeclareLaunchArgument,
  ExecuteProcess,
  IncludeLaunchDescription,
  TimerAction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import(
    Command,
    LaunchConfiguration,
    TextSubstitution,
)
from launch_ros.actions import Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
# from launch_ros.descriptions import FindPackageShare
from ros_gz_bridge.actions import RosGzBridge
from ros_gz_sim.actions import GzServer
from launch import LaunchDescription



def generate_launch_description():
    
    pkg_share = get_package_share_directory("garbo")
    default_model_path = os.path.join(pkg_share, "description", "garbo_description.sdf")
    default_rviz_config_path = os.path.join(pkg_share, "rviz", "config.rviz")
    ekf_config_path = os.path.join(pkg_share, "config", "ekf.yaml")
    twist_mux_config_path = os.path.join(pkg_share, "config", "twist_mux.yaml")
    apriltag_config_path = os.path.join(pkg_share, "config", "apriltag.yaml")
    ros_gz_sim_share = get_package_share_directory("ros_gz_sim")
    gz_spawn_model_launch_source = os.path.join(ros_gz_sim_share, "launch", "gz_spawn_model.launch.py")
    world_path = os.path.join(pkg_share, "world", "driveway.sdf")
    bridge_config_path = os.path.join(pkg_share, "config", "bridge_config.yaml")

#Variables
    camera_frame_type = LaunchConfiguration("camera_frame_type")
    camera_namespace = LaunchConfiguration("camera_namespace")
    #tag_family = LaunchConfiguration("tag_family")
    #tag_id = LaunchConfiguration("tag_id")

    declare_camera_frame_type_cmd = DeclareLaunchArgument(
        name="camera_frame_type",
        default_value="_optical_frame",
        description="Type of camera frame to use (e.g., _depth_optical_frame, _optical_frame)"
    )
    declare_camera_namespace_cmd = DeclareLaunchArgument(
        name="camera_namespace",
        default_value="depth_camera",
        description="Namespace for the camera and AprilTag nodes"
    )
    declare_tag_family_cmd = DeclareLaunchArgument(
        name="tag_family",
        default_value="tag36h11",
        description="Family of AprilTag being used"
    )
    declare_tag_id_cmd = DeclareLaunchArgument(
        name="tag_id",
        default_value="0",
        description="ID of the AprilTag being used"
    )



    robot_state_publisher_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[{"robot_description": Command(["xacro ", LaunchConfiguration("model")])}, {"use_sim_time": True}]
    )
    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=["-d", LaunchConfiguration("rvizconfig")],
        parameters=[{"use_sim_time": True}],
    )
    robot_localization_node = Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_node",
        output="screen",
        parameters=[ekf_config_path, {"use_sim_time": True}],
    )
    twist_mux = Node(
        package="twist_mux",
        executable="twist_mux",
        parameters=[twist_mux_config_path, {"use_sim_time": True}, {"use_stamped": True}],
        remappings=[("/cmd_vel_out", "/demo/cmd_vel") ],
    )
    apriltag_node = Node(
        package="apriltag_ros", # The name of the package containing the node executable
        executable="apriltag_node", # The name of the executable for the AprilTag detector
        name="apriltag_detector",
        output="screen",
        # Optional: Remap topics if necessary to match your system (e.g., camera input)
        remappings=[ 
            #("/image_rect", "/depth_camera/image_downsized"),
            ("/image_rect", "/depth_camera/image_rect"),
            #("/image_rect", "/rear_camera/image_downsized"),
            #("/image_rect", "/rear_camera/image_rect"),
            #("/camera_info", "/depth_camera/camera_info")
        ],
        parameters=[apriltag_config_path, {"use_sim_time": True}],
    )   
    #https://docs.ros.org/en/kilted/p/image_proc/doc/tutorials.html#launch-image-proc-components 
    container = ComposableNodeContainer(
        name="image_proc_container",
        namespace="",
        package="rclcpp_components",
        executable="component_container",
        parameters=[{"use_sim_time": True}],
        composable_node_descriptions=[
            ComposableNode(
                package="image_proc",
                plugin="image_proc::RectifyNode",
                name="rectify_node",
                remappings=[
                    ("image", "depth_camera/image_raw"),
                    ("image_rect", "depth_camera/image_rect"),
                    #("image", "rear_camera/image_raw"),
                    #("image_rect", "rear_camera/image_rect"),
                ],
            ),
#            ComposableNode( #maybe useful for repositioning the pointscloud image, but no idea how. 
#                    package="depth_image_proc",
#                    plugin="depth_image_proc::PointCloudXyzrgbNode",
#                    name="point_cloud_xyzrgb_node",
#                    remappings=[
#                        ("rgb/image_rect_color", "depth_camera/image_rect"),
#                        ("depth_registered/image_rect", "depth_registered/image"),
#                        ("points", "depth_registered/points"),
#                    ],
#            ),
#
#           ComposableNode( #Unnecessary?
#               package="image_proc",
#               plugin="image_proc::TrackMarkerNode",
#               name="track_marker_node",
#               parameters=[apriltag_config_path, {"use_sim_time": True}], #shares a config file with the apriltag_ros node to ensure consistent parameters
#               # Remap to match input camera topics
#               remappings=[
#                    ("image", "depth_camera/image_raw"),
#                    ("camera_info", "depth_camera/camera_info")
#              ]
#           ),
#           ComposableNode(
#               package="image_proc",
#               plugin="image_proc::CropDecimateNode",
#               name="crop_decimate_node",
#               remappings=[
#                  ("in/image_raw", "depth_camera/image_rect"),
#                   ("out/image_raw", "depth_camera/image_downsized")
#                   #("in/image_raw", "rear_camera/image_rect"),
#                   #("out/image_raw", "rear_camera/image_downsized")
#             ],
#               parameters=[{"decimation_x": 2, "decimation_y": 2,}]
#           )
        ]
    )
    start_detected_dock_pose_publisher = Node(
        package="garbo",
        executable="detected_dock_pose_publisher",
        parameters=[{
            "use_sim_time": True,
            "parent_frame": [camera_namespace, TextSubstitution(text=""), camera_frame_type],
            "child_frame": "home",
            "publish_rate": 10.0
        }],
        output="screen"
    )
    gz_server = GzServer(                   #Moved near the end to help solve a race condition where TF wasnt fully loaded before the sim started.
        world_sdf_file=world_path,
        container_name="ros_gz_container",
        create_own_container="True",
        use_composition="True",
        #arguments=["-s",],
    )
    ros_gz_bridge = RosGzBridge(
        bridge_name="ros_gz_bridge",
        config_file=bridge_config_path,
        container_name="ros_gz_container",
        create_own_container="False",
        use_composition="True",
    )
#    depth_camera_bridge_image = Node(
#        package="ros_gz_image",
#        executable="image_bridge",
#        name="bridge_gz_ros_depth_camera_image",
#        output="screen",
#        parameters=[{"use_sim_time": True}],
#        arguments=["/depth_camera/image"],
#   )
#    depth_camera_bridge_depth = Node(
#        package="ros_gz_image",
#        executable="image_bridge",
#        name="bridge_gz_ros_depth_camera_depth",
#        output="screen",
#        parameters=[{"use_sim_time": True}],
#        arguments=["/depth_camera/depth_image"],
#    )
#    depth_camera_bridge_points = Node(
#        package="ros_gz_bridge",
#        executable="parameter_bridge", #parameter_bridge?
#        name="bridge_gz_ros_depth_camera_points",
#        output="screen",
#        parameters=[{"use_sim_time": True}],
#        arguments=["/depth_camera/depth/color/points@sensor_msgs/msg/PointCloud2[gz.msgs.PointCloudPacked"],
#    )
#    rear_camera_bridge_image = Node(
#        package="ros_gz_image",
#        executable="image_bridge",
#        name="bridge_gz_ros_rear_camera_image",
#        output="screen",
#        parameters=[{"use_sim_time": True}],
#        arguments=["/rear_camera/image_raw"],
#    )
    spawn_entity = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(gz_spawn_model_launch_source),
        launch_arguments={
            "world": "driveway",
            "topic": "/robot_description",
            "entity_name": "garbo",
            "z": "1.0",
        }.items(),
    )

#Beginning of new code addition for lidar initialization
       # === NEW: Force lidar entity initialization after spawn ===
    force_lidar_init = ExecuteProcess(
        cmd=["gz", "model", "-m", "garbo", "-l", "lidar_link"],
        output="screen",
        shell=True,
    )

    # Wrap the force command with a short delay (2–3 seconds is usually enough)
    delayed_force_cmd = TimerAction(
        period=5.0,  # ← adjust if needed (2.0 ~ 4.0)
        actions=[force_lidar_init],
    )
    # Include the delayed force cmd to your launch arguemnt.
#End of new code addition for lidar initialization

    ld = LaunchDescription()
    ld.add_action(declare_camera_frame_type_cmd)
    ld.add_action(declare_camera_namespace_cmd)
    ld.add_action(declare_tag_family_cmd)
    ld.add_action(declare_tag_id_cmd)
    ld.add_action(start_detected_dock_pose_publisher)

    return LaunchDescription([
        DeclareLaunchArgument(name="model", default_value=default_model_path, description="Absolute path to robot model file"),
        DeclareLaunchArgument(name="rvizconfig", default_value=default_rviz_config_path, description="Absolute path to rviz config file"),
        #DeclareLaunchArgument(name="use_sim_time", default_value="True", description="Flag to enable use_sim_time"),
        ExecuteProcess(cmd=["gz", "sim", "-g"], output="screen"),
        robot_state_publisher_node,
        rviz_node,
        robot_localization_node,
        twist_mux,
        apriltag_node,
        container, #image_proc
        ld, #Dock detector
        gz_server,
        ros_gz_bridge,
#        depth_camera_bridge_image,
#        depth_camera_bridge_depth,
#        depth_camera_bridge_points,
#        rear_camera_bridge_image,
        spawn_entity,
        delayed_force_cmd, #Force lidar initialization after spawn
    ])