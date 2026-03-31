A differential drive robot using the usual sensors to navigate an environment. 
At this stage of the build I am trying to get the robot to dock properly, but it is never successful after the "navigate to pose" stage.
The simulation fails to load the lidar returns properly 90% of the time. The beams are generated at the correct tranform location on the bot by Gazebo and displayed properly in RVIZ2, but are SHOWN as originating from the base_footprint location in Gazebo. This feedback loop then negatively impacts navigation. This happens more frequently when the process is run from within VSCode, but still happens most of the time when run from Terminal as well. My computer is a (5 year old?) Asus Vivobook with an i5 processor and 12gb of RAM, running Ubuntu 24, ROS2 Kitled, and Gazebo Harmonic.

My launch process is my main launch file:   ros2 launch garbo display.launch.py
Then nav2 localization:   ros2 launch garbo nav2_localization_launch.py use_sim_time:=true
Then finally nav2 navigation:   ros2 launch garbo nav2_navigation_launch.py use_sim_time:=true map_subscribe_transient_local:=true

All feedback is welcome. Thank you.




*****Just a bunch of notes for myself as this is usually a private repo due to shame and embarassment****
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

Change permissions for /.bash: ~/garbo_ws/install$ chmod +x setup.bash

Custom launch command
New:
ros2 launch garbo display.launch.py
old:
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

Send docking goal:
  ros2 action send_goal /dock_robot nav2_msgs/action/DockRobot "{dock_id: <YOUR_DOCK_ID_HERE>}" --feedback

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

If Error Code 14 unable to find model file, run "export GZ_SIM_RESOURCE_PATH=/home/indie/garbo_ws/src/garbo/world/models" from the terminal in use. Add it to bashrc as well so that it loads every time.

Transform between two points
ros2 run tf2_ros tf2_echo <origin frame> <target frame>

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
-Odom is at same height as base_link, not on the ground. Fix?
-Use docking plugins>subscribe_toggle? Reduces overheard in transit, but not sure how to toggle it.
