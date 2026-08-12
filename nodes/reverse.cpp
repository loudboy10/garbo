//A node to back up the robot a set distance, then stop. Distance is an absolute value. Reverse direction is set by the speed portion of the twist message.

#include <chrono>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

using namespace std::chrono_literals;

class ReverseStamped : public rclcpp::Node {
public:
    ReverseStamped() : Node("reverse") {
        // Declare parameters for distance (meters) and speed (m/s)
        this->declare_parameter<double>("distance", 1.5);
        this->declare_parameter<double>("speed", 0.1);

        distance_ = this->get_parameter("distance").as_double();
        speed_ = this->get_parameter("speed").as_double();

        // Calculate total motion duration in seconds
        duration_ = distance_ / speed_; //Inverted because distance is negative for backing up

        publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel_keyboard", 10);
        
        // Timer running at 50Hz (20ms) to publish velocity commands
        timer_ = this->create_wall_timer(20ms, std::bind(&ReverseStamped::publish_velocity, this));
        
        // Timer to stop the robot after calculated duration
        stop_timer_ = this->create_wall_timer(
            std::chrono::duration<double>(duration_),
            std::bind(&ReverseStamped::stop_robot, this)
        );
        
        start_time_ = this->now();
        RCLCPP_INFO(this->get_logger(), "Moving robot %.2f meters at %.2f m/s", distance_, speed_);
    }

private:
    void publish_velocity() {
        auto msg = geometry_msgs::msg::TwistStamped();
        msg.header.stamp = this->now();
        msg.header.frame_id = "base_link"; // Set appropriate reference frame
        
        msg.twist.linear.x = -1 * speed_;
        msg.twist.angular.z = 0.0;
        
        publisher_->publish(msg);
    }

    void stop_robot() {
        // Publish stop message
        auto msg = geometry_msgs::msg::TwistStamped();
        msg.header.stamp = this->now();
        msg.header.frame_id = "base_link";
        msg.twist.linear.x = 0.0;
        msg.twist.angular.z = 0.0;
        publisher_->publish(msg);

        RCLCPP_INFO(this->get_logger(), "Destination reached. Stopping robot.");
        rclcpp::shutdown();
    }

    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::TimerBase::SharedPtr stop_timer_;
    rclcpp::Time start_time_;
    double distance_;
    double speed_;
    double duration_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ReverseStamped>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
