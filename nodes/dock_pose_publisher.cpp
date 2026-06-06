//T23CHIN6's version

#include <chrono>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

using namespace std::chrono_literals;

class DockPosePublisher : public rclcpp::Node {
 public:
  DockPosePublisher() : Node("dock_pose_publisher") {
    // Parameters for easy adjustment
    this->declare_parameter("target_frame", "green_bin"); //Same as child frame in original. "tag3611:0"
    this->declare_parameter("base_frame", "depth_camera_optical_frame"); //Same as parent frame in original. "camera_image_frame"
    this->declare_parameter("output_topic", "detected_dock_pose");

    target_frame_ = this->get_parameter("target_frame").as_string();
    base_frame_ = this->get_parameter("base_frame").as_string();

    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    publisher_ = this->create_publisher<geometry_msgs::msg::PoseStamped>(
        this->get_parameter("output_topic").as_string(), 10);

    // Run at 10Hz
    timer_ = this->create_wall_timer(
        100ms, std::bind(&DockPosePublisher::timer_callback, this));

    RCLCPP_INFO(this->get_logger(), "Dock Pose Publisher started. Tracking: %s",
                target_frame_.c_str());
  }

 private:
  void timer_callback() {
    geometry_msgs::msg::TransformStamped transform_stamped;

    try {
      // Look up transform from odom to the tag
      transform_stamped = tf_buffer_->lookupTransform(
          base_frame_, target_frame_, tf2::TimePointZero);
    } catch (const tf2::TransformException& ex) {
      RCLCPP_DEBUG(this->get_logger(), "Could not get transform: %s",
                   ex.what());
      return;
    }

    geometry_msgs::msg::PoseStamped dock_pose;
    dock_pose.header.stamp = transform_stamped.header.stamp;
    dock_pose.header.frame_id = base_frame_;

    // Extract position
    dock_pose.pose.position.x = transform_stamped.transform.translation.x;
    dock_pose.pose.position.y = transform_stamped.transform.translation.y;
    dock_pose.pose.position.z = transform_stamped.transform.translation.z;

    // Extract orientation
    dock_pose.pose.orientation = transform_stamped.transform.rotation;

    publisher_->publish(dock_pose);
  }

  std::string target_frame_;
  std::string base_frame_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr publisher_;
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DockPosePublisher>());
  rclcpp::shutdown();
  return 0;
}