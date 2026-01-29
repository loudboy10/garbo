GarBo, the garbage robot

"Cheaper than a broken hip!"


Templated from joshnewans/my_bot

ArticulatedRobotics.com

Right Hand Rule:
X+ is index finger pointing forward
Y+ is middle finger pointing left
Z+ is thumb pointing up

Sourcing is performed automatically via ~/.bashrc add-ons, each time a terminal is opened. An alias "source" was also added so that the workspace can be sourced from within a open terminal without having to open and close it after file changes are made.

Rebuild the workspace from /garbo_ws
colcon build --symlink-install
This version of build requires fewer rebuilds between file changes. Rebuild only required when new files are added to the package.
This has the alias "build".

Custom launch command
  ros2 launch garbo launch_sim.launch.py world:=src/garbo/worlds/Driveway/driveway.sdf  use_sim_time:=true
  -Add "headless:=True" to run Gazebo without the GUI

Teleop command with remapped output for use with twist_mux
  ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -p stamped:=true --remap cmd_vel:=/cmd_vel_keyboard

Launch SLAM to mapping mode
  Custom:
  ros2 launch slam_toolbox online_async_launch.py use_sim_time:=true params_file:=./src/garbo/config/mapper_params_online_async.yaml
  Default:
  ros2 launch slam_toolbox online_async_launch.py use_sim_time:=true

Launch SLAM to localization mode (after changing params file)
  ros2 launch garbo slam_localization_launch.py

Launch nav2 localization
  Custom:
  ros2 launch garbo nav2_localization_launch.py use_sim_time:=true
  Default:
  ros2 launch nav2_bringup localization_launch.py

Launch nav2 navigation
  Custom:
  ros2 launch garbo nav2_navigation_launch.py use_sim_time:=true map_subscribe_transient_local:=true
  Default:
  ros2 launch nav2_bringup navigation_launch.py use_sim_time:=true map_subscribe_transient_local:=true

Git Push Process
  cd ~/garbo_ws/src/garbo
  git init
  git add .
  git commit -m "comments"
  git push origin main

if spawner takes too long and times out:
https://beta.articulatedrobotics.xyz/tutorials/mobile-robot/applications/ros2_control-concepts/#updating-the-launch-file

Topic/Node Info commands
  ros2 topic list
  ros2 node list
  ros2 topic info /{topic} -v
  ros2 topic echo
  ros2 run tf2_tools view_frames
  
  To load a yaml map:
    -Open just RVIZ, not Gazebo or robot launch file
    -Edit nav2 params to point to map yaml
    - ros2 launch garbo nav2_localization_launch.py 
    -Launch slam toolbox in mapping mode (edit yaml)
    - ros2 launch slam_toolbox online_async_launch.py use_sim_time:=true
    -Set initial pose
    -Open SlamToolboxPlugin
    -Serialize to name of your choice

Then to load the serialized map
-Change SLAM config yaml to localization, set map location, set starting pose
-Remove yaml map location from nav2 config yaml
-start gazebo, robot state, slam, nav. rviz

To kill all processes
pkill -9 -f "ros2|gazebo|gz|nav2|amcl|bt_navigator|nav_to_pose|rviz2|assisted_teleop|cmd_vel_relay|robot_state_publisher|joint_state_publisher|move_to_free|mqtt|autodock|cliff_detection|moveit|move_group|basic_navigator"

To-Do
-Confirm sizes/weights/speeds for robot_core
-Set up joystick control
-Reduced obstacle range in nav2 global and local costmaps to get around ramp/lidar issue. Keepout filters are set up and run but arent achieving the desired goals and are vestigial at this point. This issue might not even be a problem IRL?
-Update collision monitor to use depth camera points instead of scan?
-Tune costmap resolution to balance computation load and not running over a cat. Global and local can be set different
-Use synchronous SLAM mode for mapping next time. Slower but better.
-In sim, use behavior tree, AprilTags, docking server, and dynamic footprint to handle picking up the bin. 
  -IRL, use above along with Arduino to handle hoist actions based on limit switches, with overrides sent to twist-mux until bin is lifted. This avoids the clusterfuck of having to simulate contact switches.
-Adjust twist-mux timeouts if commands start conflicting. Increase a timeout to block other inputs for longer.

1/2/26 - Updated a ton of Nav2 params per Addison tuning guide.
Simulation speed is much better but bot performance is worse. 
Bot is moving too fast and scans arent localizing well.
Compare params against the last commit, which navigated well but sim was slow. Good luck remembering how to do anything, fucker.