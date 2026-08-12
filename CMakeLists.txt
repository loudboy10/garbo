cmake_minimum_required(VERSION 3.5)
project(garbo)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17   17 is needed for behaviortree. Maybe need to change back to 14 for apriltag_ros?
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17) #Changed from 14 to 17 for behaviortree_cpp
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(apriltag_ros REQUIRED)
find_package(apriltag_msgs REQUIRED)
find_package(image_proc REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(nav2_msgs REQUIRED) # For checking docking state
find_package(behaviortree_cpp REQUIRED) #For BT nodes

add_executable(detected_dock_pose_publisher nodes/detected_dock_pose_publisher.cpp)
ament_target_dependencies(detected_dock_pose_publisher
  rclcpp
  geometry_msgs
  tf2_ros
  nav2_msgs #For checking docking state
)

add_executable(rotate nodes/rotate.cpp)
ament_target_dependencies(rotate
  rclcpp
  geometry_msgs
  tf2_ros
  nav2_msgs #For checking docking state
)

add_executable(reverse nodes/reverse.cpp)
ament_target_dependencies(reverse
  rclcpp
  geometry_msgs
  tf2_ros
  nav2_msgs
)

add_executable(bt_demo nodes/bt_demo.cpp)
##Original used: target_link_libraries(bt_demo PRIVATE behaviortree_cpp::behaviortree_cpp) #Link the BT library to the bt_demo executable
##Modern CMake uses ament_target_dependencies instead of target_link_libraries for linking dependencies in ROS 2 packages. This is because ament_target_dependencies automatically handles the include directories, libraries, and other necessary settings for the specified dependencies. It simplifies the process of linking dependencies and ensures that all required components are correctly set up for the target.
ament_target_dependencies(bt_demo
  rclcpp
  behaviortree_cpp
)

install(TARGETS  #cpp executables
  detected_dock_pose_publisher
  rotate
  reverse
  bt_demo
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

install(
  DIRECTORY behavior_tree config description launch nodes rviz world
  DESTINATION share/${PROJECT_NAME}
)

ament_package()