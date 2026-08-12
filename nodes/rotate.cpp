#include <chrono>
#include <cmath>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"

using namespace std::chrono_literals;

class RotateNode : public rclcpp::Node
{
public:
    RotateNode() : Node("rotate_node"), start_yaw_(0.0), target_yaw_(0.0), rotating_(false), angle_turned_(0.0)
    {
    // Declare parameters with default values and documentation
        this->declare_parameter("rotation_angle", M_PI); // How far to rotate in radians
        rotation_angle_ = this->get_parameter("rotation_angle").as_double();


        publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("cmd_vel_keyboard", 10);
        subscription_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", 10, std::bind(&RotateNode::odom_callback, this, std::placeholders::_1));
        
        timer_ = this->create_wall_timer(50ms, std::bind(&RotateNode::control_loop, this));
        
        RCLCPP_INFO(this->get_logger(), "RotateNode has been initialized. Waiting for first odom...");
    }

private:
    double rotation_angle_;

    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
    {
        // Extract orientation in quaternions
        tf2::Quaternion q(
            msg->pose.pose.orientation.x,
            msg->pose.pose.orientation.y,
            msg->pose.pose.orientation.z,
            msg->pose.pose.orientation.w
        );
        
        // Convert to Euler angles
        tf2::Matrix3x3 m(q);
        double roll, pitch, yaw;
        m.getRPY(roll, pitch, yaw);
        
        current_yaw_ = yaw;

        if (!rotating_) {
            start_yaw_ = current_yaw_;
            target_yaw_ = start_yaw_ + rotation_angle_; // 180 degrees in radians

            // Normalize target_yaw_ to be between -pi and pi
            if (target_yaw_ > M_PI) {
                target_yaw_ -= 2 * M_PI;
            }
            rotating_ = true;
            RCLCPP_INFO(this->get_logger(), "Starting rotation. Target Yaw: %.2f", target_yaw_);
        }
    }

    void control_loop()
    {
        if (!rotating_) {
            return;
        }

        auto twist_msg = geometry_msgs::msg::TwistStamped();
        twist_msg.header.stamp = this->get_clock()->now();
        twist_msg.header.frame_id = "base_link";
        double error = target_yaw_ - current_yaw_;

        // Normalize error
        if (error > M_PI) error -= 2 * M_PI;
        else if (error < -M_PI) error += 2 * M_PI;

        double kp = 1.0; // Proportional gain
        double angular_speed_limit = 0.5; // Max angular velocity

        if (std::abs(error) > 0.05) { // 0.05 rad tolerance (~2.8 degrees)
            twist_msg.twist.angular.z = std::min(std::max(kp * error, -angular_speed_limit), angular_speed_limit);
        } else {
            twist_msg.twist.angular.z = 0.0;
            rotating_ = false;
            RCLCPP_INFO(this->get_logger(), "Rotation completed!");
            rclcpp::shutdown();
        }

        publisher_->publish(twist_msg);
    }

    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr publisher_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr subscription_;
    rclcpp::TimerBase::SharedPtr timer_;
    
    double current_yaw_;
    double start_yaw_;
    double target_yaw_;
    bool rotating_;
    double angle_turned_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RotateNode>());
    rclcpp::shutdown();
    return 0;
}
