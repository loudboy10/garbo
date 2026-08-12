/** Still need to remove the dock publisher components and leave only the EKF stuff. Still not sure if I can use this????
 * 
 * https://kjdotio-innex1-rover.mintlify.app/components/localisation
 * https://github.com/KJdotIO/innex1-rover/blob/main/src/lunabot_localisation/lunabot_localisation/tag_pose_publisher.py
 * 
 * This program detects an AprilTag that is mounted at a known and fixed location,
 * and publishes its pose relative to the camera optical frame.
 * The optical frame follows the typical computer vision convention where:
 *   - Z forward (pointing out from the camera)
 *   - X right
 *   - Y down
 *
 * The node subscribes to TF2 transforms published by the AprilTag detection system between the
 * camera's optical frame and the detected tag's frame. It then republishes these transforms as
 * PoseWithCovarianceStamped messages that can be used by the the robot_localization package and EKF
 * to correct odometry drift and improve localization.
 *
 * Subscription Topics:
 *     /tf (tf2_msgs/TFMessage): Transform tree containing camera optical frame to tag transforms
 *
 * Publishing Topics:
 *     /EKF_dock_pose (geometry_msgs/PoseWithCovarianceStamped): Pose of the detected AprilTag relative to the camera optical frame, with a covariance matrix
 *
 * Parameters:
 *     parent_frame (string, default: "cam_1_depth_optical_frame"): Name of the camera's optical frame
 *     child_frame (string, default: "tag36h11:0"): Name of the AprilTag frame
 *     publish_rate (double, default: 10.0): How often to publish the tag pose in Hz
 *
 * Modified from AutomaticAddison.com, Addison Sears-Collins
 *
 */

#include <memory>
#include <string>
#include <iostream>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "nav2_msgs/action/dock_robot.hpp" // For checking docking state

/**
 * A ROS2 node that publishes AprilTag poses relative to the camera optical frame
 *
 * This node listens for transforms between the camera's optical frame (cam_1_depth_optical_frame)
 * and an AprilTag frame (tag36h11:0). The optical frame is important as it follows standard
 * computer vision conventions and is the frame in which the AprilTag detector operates.
 *
 * The AprilTag is mounted near or on a docking station, and this node publishes the tag's pose
 * in the optical frame coordinate system. The Nav2 docking system can then use this tag pose
 * as a reference to compute the actual docking position.
 */
class LandmarkPosePublisher : public rclcpp::Node
{
public:
  /**
   * Constructor for the LandmarkPosePublisher node
   *
   * Initializes the node, sets up parameters, and creates publishers and transform listeners
   */
  using DockRobotFeedbackMsg = nav2_msgs::action::DockRobot_FeedbackMessage;  
  LandmarkPosePublisher()
  : Node("landmark_pose_publisher")
  {
    // Declare parameters with default values and documentation
    this->declare_parameter("parent_frame", "depth_camera_optical_frame"); //Reference frame default "depth_camera_optical_frame"
    this->declare_parameter("child_frame", "home_ID"); // Default to the tag_ID name (NOT THE DOCK NAME!), can be overridden for other tags
    this->declare_parameter("publish_rate", 10.0);  // Hz
    this->declare_parameter("docking_state", 0); //for checking docking state

    // Get the values of our parameters
    parent_frame_ = this->get_parameter("parent_frame").as_string();
    child_frame_ = this->get_parameter("child_frame").as_string();
    double publish_rate = this->get_parameter("publish_rate").as_double();

    //Register the child_frame parameter callback
    param_callback_handle_ = this->add_on_set_parameters_callback(
      std::bind(&LandmarkPosePublisher::paramCallback, this, std::placeholders::_1));

    // Subscribe to the docking server action feedback topic
    feedback_sub_ = this->create_subscription<DockRobotFeedbackMsg>(
      "/dock_robot/_action/feedback",
      rclcpp::SystemDefaultsQoS(),
      std::bind(&LandmarkPosePublisher::feedback_callback, this, std::placeholders::_1)
    );

    // Create a transform buffer to store and look up transforms
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());

    // Create a transform listener to receive transforms
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // Create a publisher for the dock pose
    dock_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>(
      "detected_dock_pose", 10);

    // Create a publisher for the EKF pose
    EKF_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "EKF_dock_pose", 10);

    // Create a timer that will trigger pose updates
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(static_cast<int>(1000.0 / publish_rate)),
      std::bind(&LandmarkPosePublisher::timer_callback, this));

    // Log that we've successfully initialized
    RCLCPP_INFO(this->get_logger(),
      "Landmark pose publisher initialized with parent frame: '%s' and child frame: '%s'",
      parent_frame_.c_str(), child_frame_.c_str());
  }

    //Make docking state available elsewhere in the node
    uint16_t get_docking_state() const{
    return docking_state_;
    }

private:

  //Action subscriber to chcek dock state
    rclcpp::Subscription<DockRobotFeedbackMsg>::SharedPtr feedback_sub_;
  
  // Stored state variable updated automatically when any external client commands a dock
  uint16_t docking_state_;

  void feedback_callback(const DockRobotFeedbackMsg::SharedPtr msg)
  {
    // The actual feedback payload lives inside the '.feedback' field of the topic wrapper
    docking_state_ = msg->feedback.state;

    RCLCPP_INFO(this->get_logger(), "Passive State Catch -> Current Docking State: %u", docking_state_);
  }


  //Callback function to handle parameter changes
  rcl_interfaces::msg::SetParametersResult paramCallback(
    const std::vector<rclcpp::Parameter> & parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    result.reason = "";

    for (const auto & param : parameters)
    {
      if (param.get_name() == "child_frame")
      {
        if (param.get_type() == rclcpp::ParameterType::PARAMETER_STRING)
        {
          child_frame_ = param.as_string();
          RCLCPP_INFO(this->get_logger(), "Updated child_frame to: '%s'", child_frame_.c_str());
        }
        else
        {
          result.successful = false;
          result.reason = "child_frame must be a string";
        }
      }
    }
    return result;
  }

  std::string child_frame_;
  OnSetParametersCallbackHandle::SharedPtr param_callback_handle_;


  /**
   * Timer callback that publishes the latest dock pose
   *
   * This function is called periodically to:
   * 1. Look up the latest transform between the camera and AprilTag
   * 2. Convert the transform into a pose message
   * 3. Publish the pose for the docking system to use
   */

  void timer_callback()
  {
    // Create a new pose message
    geometry_msgs::msg::PoseStamped dock_pose;
    // Set the timestamp to now
    dock_pose.header.stamp = this->get_clock()->now();
    // The frame ID should match the frame we want the pose expressed in
    dock_pose.header.frame_id = "map"; //parent_frame_;

    // Create a new EKF message
    geometry_msgs::msg::PoseWithCovarianceStamped EKF_pose;
    // Set the timestamp to now
    EKF_pose.header.stamp = this->get_clock()->now();
    // The frame ID should match the frame we want the pose expressed in
    EKF_pose.header.frame_id = parent_frame_;
    //Set the covarience for the message. This should always be zero (because the tag position is known?)
    EKF_pose.pose.covariance = {
    0.01, 0.0,  0.0,  0.0,  0.0,  0.0,
    0.0,  0.01, 0.0,  0.0,  0.0,  0.0,
    0.0,  0.0,  0.01, 0.0,  0.0,  0.0,
    0.0,  0.0,  0.0,  0.01, 0.0,  0.0,
    0.0,  0.0,  0.0,  0.0,  0.01, 0.0,
    0.0,  0.0,  0.0,  0.0,  0.0,  0.01
    };

    try {

      // Look up the transform to the pose
      geometry_msgs::msg::TransformStamped transform = tf_buffer_->lookupTransform(
        "map",  //parent_frame_,
        child_frame_,
        tf2::TimePointZero // get latest transform
      );

      // Copy the translation from the transform to the pose for docking
      dock_pose.pose.position.x = transform.transform.translation.x;
      dock_pose.pose.position.y = transform.transform.translation.y;
      dock_pose.pose.position.z = transform.transform.translation.z;

      // Copy the rotation from the transform to the pose
      dock_pose.pose.orientation = transform.transform.rotation;

      // Publish the dock pose for the navigation system to use
//Stops the redundant message from being sent      dock_pose_pub_->publish(dock_pose);


      // Copy the translation from the transform to the pose for EKF (Covariance messages are another layer down, hence the extra 'pose'??????)
      EKF_pose.pose.pose.position.x = transform.transform.translation.x;
      EKF_pose.pose.pose.position.y = transform.transform.translation.y;
      EKF_pose.pose.pose.position.z = transform.transform.translation.z;

      // Copy the rotation from the transform to the pose
      EKF_pose.pose.pose.orientation = transform.transform.rotation;

      // Publish the dock pose for the robot_localization package to use
      EKF_pose_pub_->publish(EKF_pose);
    }

    catch (const tf2::TransformException & ex) {
      // If we can't get the transform, log it at debug level to avoid spamming
      RCLCPP_DEBUG(this->get_logger(), "Could not get transform: %s", ex.what());
      return;
    }
  }

  // Frame names from parameters
  std::string parent_frame_; ///< Name of the camera frame
  //Already declared at line 128:   std::string child_frame_;  ///< Name of the AprilTag frame

  // ROS infrastructure
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;        ///< Buffer for storing transforms
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_; ///< Listener for transforms
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr dock_pose_pub_; ///< Publisher for dock poses
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr EKF_pose_pub_; ///< Publisher for EKF poses
  rclcpp::TimerBase::SharedPtr timer_;               ///< Timer for periodic publishing
};

/**
 * @brief Main function that starts the landmark pose publisher node
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return int Exit code (0 if successful)
 */
int main(int argc, char * argv[])
{
  // Initialize ROS
  rclcpp::init(argc, argv);
  // Create and spin (run) the node
  rclcpp::spin(std::make_shared<LandmarkPosePublisher>());
  auto node = std::make_shared<LandmarkPosePublisher>();
  // Clean up ROS and exit
  rclcpp::shutdown();
  return 0;
}