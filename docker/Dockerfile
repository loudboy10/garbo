ARG ROS_DISTRO=jazzy
FROM osrf/ros:${ROS_DISTRO}-desktop-full

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=${ROS_DISTRO}
ENV GZ_VERSION=harmonic

RUN apt-get update && apt-get install -y \
    ros-dev-tools \
    ros-${ROS_DISTRO}-navigation2 \
    ros-${ROS_DISTRO}-nav2-bringup \
    ros-${ROS_DISTRO}-ros-gz \
    python3-colcon-common-extensions \
    ros-${ROS_DISTRO}-twist-mux \
    ros-${ROS_DISTRO}-apriltag-ros \
    && rm -rf /var/lib/apt/lists/*

# Set up the workspace
WORKDIR /ros2_ws

# Setup entrypoint to source ROS2 environment
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> ~/.bashrc

ENV DEBIAN_FRONTEND=

ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["bash"]
