#include <memory>
#include <string>
#include <map>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "opennav_docking_msgs/action/dock_robot.hpp"

class MultiDockMux : public rclcpp::Node
{

public:
  using DockRobot = opennav_docking_msgs::action::DockRobot;

  MultiDockMux()
  : Node("multi_dock_pose_publisher")
  {
    // 1. Map Dock IDs (from docks.yaml) to AprilTag TF frames
    dock_to_tag_map_ = {
      {"home", "home_ID"},
      {"trash_bin", "trash_bin_ID"},
      {"recycle_bin", "recycle_bin_ID"},
      {"green_bin", "green_bin_ID"},
      {"trash_dock", "bin_dock_ID"}, //These two are the same because they referene the same external anchor tag for locating.
      {"recycle_dock", "bin_dock_ID"}, //These two are the same because they referene the same external anchor tag for locating.
      //{"dock_station_beta", "tag_36h11_1"}
    };

    // 2. Initialize TF buffer and listener
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::Transform_Listener>(*tf_buffer_);

    // 3. Publisher for Nav2 Docking Server
    pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("detected_dock_pose", 10);

    // 4. Subscriber to monitor which dock is being requested
    // Note: In production, you'd typically intercept the Action Goal or 
    // monitor the '/dock_robot/_action/status' to set active_tag_frame_
    goal_sub_ = this->create_subscription<DockRobot::Goal>(
      "/dock_robot/_action/goal", 10, 
      std::bind(&MultiDockMux::goal_callback, this, std::placeholders::_1));

    // 5. High-frequency timer for the vision loop (e.g., 10Hz)
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(100), std::bind(&MultiDockMux::publish_dock_pose, this));
  }

private:
  void goal_callback(const std::shared_ptr<DockRobot::Goal> msg) {
    if (dock_to_tag_map_.count(msg->dock_id)) {
      active_tag_frame_ = dock_to_tag_map_[msg->dock_id];
      RCLCPP_INFO(this->get_logger(), "Active dock set to: %s (Targeting tag: %s)", 
                  msg->dock_id.c_str(), active_tag_frame_.c_str());
    }
  }

  void publish_dock_pose() {
    if (active_tag_frame_.empty()) return;

    geometry_msgs::msg::PoseStamped pose_msg;
    try {
      // Lookup transform from camera to the specific AprilTag
      auto tf = tf_buffer_->lookupTransform(
        "odom", active_tag_frame_, tf2::TimePointZero); //Originally "depth_camera_optical_frame" but changed so that the dock pose would stay constant if visual lock was lost.

      pose_msg.header.stamp = this->now();
      pose_msg.header.frame_id = "odom"; //Originally "depth_camera_optical_frame" but changed so that the dock pose would stay constant if visual lock was lost.
      pose_msg.pose.position.x = tf.transform.translation.x;
      pose_msg.pose.position.y = tf.transform.translation.y;
      pose_msg.pose.position.z = tf.transform.translation.z;
      pose_msg.pose.orientation = tf.transform.rotation;

      pose_pub_->publish(pose_msg);
    } catch (const tf2::TransformException & ex) {
      // Silently skip if tag isn't in view
    }
  }

  std::map<std::string, std::string> dock_to_tag_map_;
  std::string active_tag_frame_;
  
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_;
  rclcpp::Subscription<DockRobot::Goal>::SharedPtr goal_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MultiDockMux>());
  rclcpp::shutdown();
  return 0;
}
