//Example BT node with dummy functions to demonstrate how to create a BT node using the BehaviorTree.CPP library. This example includes a standard class style node, a function style node, and a custom class style node. The nodes are registered with the BehaviorTreeFactory and executed in a simple behavior tree.
//https://www.youtube.com/watch?v=4PUiDmD5dkg


#include <iostream>
#include <chrono>
#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/bt_factory.h"

using namespace std::chrono_literals;

//Standard class stype example method for a BT node. This node will approach an object for 5 seconds and then return success.
//(It doesnt actually check anything yet, it just prints a message to the console and sleeps for 5 seconds.)
class ApproachObject : public BT::SyncActionNode
{
public:
  explicit ApproachObject(const std::string &name) : BT::SyncActionNode(name, {})
  {
  }

  BT::NodeStatus tick() override
  {
    std::cout << "Approach Object: " << this->name() << std::endl;

    std::this_thread::sleep_for(5s);
    return BT::NodeStatus::SUCCESS;
  }
};

//Function style example method for a BT node. This node will check the battery level and return success if the battery is OK.
//(It doesnt actually check anything yet, it just prints a message to the console and returns success.)
BT::NodeStatus CheckBattery()
{
  std::cout << "Battery OK" << std::endl;
  return BT::NodeStatus::SUCCESS;
}

//Custom class style example method for a BT node. This node will open and close the gripper.
//(It doesnt actually check anything yet, it just prints a message to the console and returns success.)
class GripperInterface
{
public:
  GripperInterface() : _open(true) {}

  BT::NodeStatus open()
  {
    _open = true;
    std::cout << "Gripper open" << std::endl;
    return BT::NodeStatus::SUCCESS;
  }

  BT::NodeStatus close()
  {
    _open = false;
    std::cout << "Gripper close" << std::endl;
    return BT::NodeStatus::SUCCESS;
  }


private: 
  bool _open; //This stores the state of the gripper.  Open is true, closed is false.  This is just for demonstration purposes and does not actually control a gripper.
};

int main()
{
  BT::BehaviorTreeFactory factory; //This is where the above actions are registered with the BehaviorTree. This creates them.
  //Add the actions here in the order they are written above. Names here must match the names in the BT XML file.

  factory.registerNodeType<ApproachObject>("ApproachObject");

  factory.registerSimpleCondition("CheckBattery", std::bind(CheckBattery));

  GripperInterface gripper;

  factory.registerSimpleAction(
      "OpenGripper",
      std::bind(&GripperInterface::open, &gripper));

  factory.registerSimpleAction(
      "CloseGripper",
      std::bind(&GripperInterface::close, &gripper));

  //Create Tree
  auto tree = factory.createTreeFromFile("/home/indie/garbo_ws/src/garbo/behavior_tree/bt_tree.xml"); //example was "./../bt_tree.xml", but cpp, xml, and CMakeList are all in the same folder, so I changed it to "./../../behavior_tree/bt_tree.xml" to match the file structure of this project.

  //execute the tree
  tree.tickWhileRunning();

  return 0;
}