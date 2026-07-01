This repo is for building a differential drive robot that navigates with lidar and depth camera, and docks to various points using AprilTag reference points.

It is a beginner hobbiest project based on Nav2 Sam_bot, Josh Newan's ArticulatedRobots.xyz tutorials, and AutomaticAddison.com tutorials.

Currently the main launch file, Nav2 localization launch file, and Nav2 navigation launch file are all separate for testing and my own learning.

Main launch with: ros2 launch garbo display.launch.py

Nav2 Localization launch: ros2 launch garbo nav2_localization_launch.py use_sim_time:=true

Nav2 Navigation launch: ros2 launch garbo nav2_navigation_launch.py use_sim_time:=true map_subscribe_transient_local:=true

To change the intended dock:
ros2 param set /detected_dock_pose_publisher child_frame <dock_ID_here>

Followed by the dock command. Note that the dock_ID and dock name are not the same, and dock_ID will not work here. (If I misundersand something about this I would love to know):
ros2 action send_goal /dock_robot nav2_msgs/action/DockRobot "{dock_id: <dock_name>}" --feedback


The depth camera /depthcloud topic is remapped to /pointcloud because the orientations will not agree. The camera optical frame is rotated so that the camera image is correct, but doing so makes the pointcloud image rotated out of aligment by (0, 1.57, 1.57). If there is a proper way to get pointcloud to aligne with optical frame, I'd love to know.

The robot docks properly in the forward direction to an Apriltag or to a map pose. The robot docks backward to a map pose but I have not been able to figure out docking backward to an Apriltag. When attempting backward docking to an Apriltag, the robot stages properly and /dock_pose_publisher send the dock pose. The robot then executes its 180* spin and promptly loses its mind. The dock pose icon jumps back to in front of the robot and stays locked in that relative location as the robot spins indefinitely. Help?