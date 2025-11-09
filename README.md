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

Teleop command
  ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -p stamped:=true

Launch SLAM to mapping mode
  ros2 launch slam_toolbox online_async_launch.py use_sim_time:=true params_file:=./src/garbo/config/mapper_params_online_async.yaml

Launch SLAM to localization mode (after changing params file)
  ros2 launch garbo slam_localization_launch.py

Launch nav2 localization
  ros2 launch garbo nav2_localization_launch.py
  ros2 launch nav2_bringup localization_launch.py

Launch nav2 navigation
  ros2 launch garbo navigation_launch.py use_sim_time:=true map_subscribe_transient_local:=true
  ros2 launch nav2_bringup navigation_launch.py use_sim_time:=true map_subscribe_transient_local:=true

Git Push Process
  cd ~/garbo_ws/src/garbo
  git init
  git add .
  git commit -m "comments"
  git push origin main

if spawner takes too long and times out:
https://beta.articulatedrobotics.xyz/tutorials/mobile-robot/applications/ros2_control-concepts/#updating-the-launch-file

Is nav2_behaviors node needed in navigation_launch?