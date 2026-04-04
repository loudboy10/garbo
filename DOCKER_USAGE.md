# Docker Usage Guide

## Overview
This guide explains how to use Docker and Docker Compose to build and run the development environment.

## Prerequisites
- Docker installed
- Docker Compose installed
- NVIDIA container toolkit installed

## Building the Environment

### Using Docker Compose
```bash
cd ~/ros2_ws/src/garbo/docker
docker compose up --build
```

## Running the Development Environment

### Start Services in Background
```bash
cd ~/ros2_ws/src/garbo/docker
docker-compose up -d
```

### Using Docker Directly
```bash
cd ~/ros2_ws/src/garbo/docker
docker compose exec ros2_dev bash
```

### Build the Workspace (Inside the containter)

```bash
cd ~/ros2_ws
colcon build --symlink-install
```

## Stopping the Environment

```bash
cd ~/ros2_ws/src/garbo/docker
docker-compose down
```
