#include "justina_tools/JustinaHardware.h"

bool JustinaHardware::is_node_set = false;
//Publishers and subscribers for operating the head
ros::Subscriber JustinaHardware::subHeadCurrentPose;
ros::Publisher JustinaHardware::pubHeadGoalPose;
//Publishers and subscribers for operating left arm
ros::Subscriber JustinaHardware::subLeftArmCurrentGripper;
ros::Subscriber JustinaHardware::subLeftArmCurrentPose;
ros::Publisher JustinaHardware::pubLeftArmGoalGripper;
ros::Publisher JustinaHardware::pubLeftArmGoalPose;
ros::Publisher JustinaHardware::pubLeftArmGoalTorqueGrip;
ros::Publisher JustinaHardware::pubLeftArmGoalTorque;
//Publishers and subscribers for operating right arm
ros::Subscriber JustinaHardware::subRightArmCurrentGripper;
ros::Subscriber JustinaHardware::subRightArmCurrentPose;
ros::Publisher JustinaHardware::pubRightArmGoalGripper;
ros::Publisher JustinaHardware::pubRightArmGoalPose;
ros::Publisher JustinaHardware::pubRightArmGoalTorqueGrip;
ros::Publisher JustinaHardware::pubRightArmGoalTorque;
//Publishers and subscribers for operating mobile base
ros::Publisher JustinaHardware::pubBaseSpeeds;
ros::Publisher JustinaHardware::pubBaseCmdVel;
//Publishers and subscribers for checking robot state
ros::Subscriber JustinaHardware::subBaseBattery;
ros::Subscriber JustinaHardware::subLeftArmBattery;
ros::Subscriber JustinaHardware::subRightArmBattery;
ros::Subscriber JustinaHardware::subHeadBattery;

//Variables for head position
float JustinaHardware::headPan;
float JustinaHardware::headTilt;
//Variables for arms
float JustinaHardware::leftArmCurrentGripper;
float JustinaHardware::rightArmCurrentGripper;
std::vector<float> JustinaHardware::leftArmCurrentPose;
std::vector<float> JustinaHardware::rightArmCurrentPose;
//Variables for robot state;
float JustinaHardware::baseBattery = 0;
float JustinaHardware::leftArmBattery = 0;
float JustinaHardware::rightArmBattery = 0;
float JustinaHardware::headBattery = 0;

bool JustinaHardware::setNodeHandle(ros::NodeHandle* nh)
{
    if(JustinaHardware::is_node_set)
        return true;
    if(nh == 0)
        return false;

    //Publishers and subscribers for operating the head
    JustinaHardware::subHeadCurrentPose = nh->subscribe("/hardware/head/current_pose", 1, &JustinaHardware::callbackHeadCurrentPose);
    JustinaHardware::pubHeadGoalPose = nh->advertise<std_msgs::Float32MultiArray>("/hardware/head/goal_pose", 1);
    //Publishers and subscribers for operating left arm
    JustinaHardware::subLeftArmCurrentGripper = nh->subscribe("/hardware/left_arm/current_gripper", 1, &JustinaHardware::callbackLeftArmCurrentGripper);
    JustinaHardware::subLeftArmCurrentPose = nh->subscribe("/hardware/left_arm/current_pose", 1, &JustinaHardware::callbackLeftArmCurrentPose);
    JustinaHardware::pubLeftArmGoalGripper = nh->advertise<std_msgs::Float32>("/hardware/left_arm/goal_gripper", 1);
    JustinaHardware::pubLeftArmGoalPose = nh->advertise<std_msgs::Float32MultiArray>("/hardware/left_arm/goal_pose", 1);
    JustinaHardware::pubLeftArmGoalTorqueGrip = nh->advertise<std_msgs::Float32>("/hardware/left_arm/torque_gripper", 1);
    JustinaHardware::pubLeftArmGoalTorque = nh->advertise<std_msgs::Float32MultiArray>("/hardware/left_arm/goal_torque", 1);
    //Publishers and subscribers for operating right arm
    JustinaHardware::subRightArmCurrentGripper = nh->subscribe("/hardware/right_arm/current_gripper", 1, &JustinaHardware::callbackRightArmCurrentGripper);
    JustinaHardware::subRightArmCurrentPose = nh->subscribe("/hardware/right_arm/current_pose", 1, &JustinaHardware::callbackRightArmCurrentPose);
    JustinaHardware::pubRightArmGoalGripper = nh->advertise<std_msgs::Float32>("/hardware/right_arm/goal_gripper", 1);
    JustinaHardware::pubRightArmGoalPose = nh->advertise<std_msgs::Float32MultiArray>("/hardware/right_arm/goal_pose", 1);
    JustinaHardware::pubRightArmGoalTorqueGrip = nh->advertise<std_msgs::Float32>("/hardware/right_arm/torque_gripper", 1);
    JustinaHardware::pubRightArmGoalTorque = nh->advertise<std_msgs::Float32MultiArray>("/hardware/right_arm/goal_torque", 1);
    //Publishers and subscribers for operating mobile base
    JustinaHardware::pubBaseSpeeds = nh->advertise<std_msgs::Float32MultiArray>("/hardware/mobile_base/speeds", 1);
    JustinaHardware::pubBaseCmdVel = nh->advertise<geometry_msgs::Twist>("/hardware/mobile_base/cmd_vel", 1);
    //Publishers and subscribers for checking robot state
    JustinaHardware::subBaseBattery = nh->subscribe("/hardware/robot_state/base_battery", 1, &JustinaHardware::callbackBaseBattery);
    JustinaHardware::subLeftArmBattery = nh->subscribe("/hardware/robot_state/left_arm_battery", 1, &JustinaHardware::callbackLeftArmBattery);
    JustinaHardware::subRightArmBattery = nh->subscribe("/hardware/robot_state/right_arm_battery", 1, &JustinaHardware::callbackRightArmBattery);
    JustinaHardware::subHeadBattery = nh->subscribe("/hardware/robot_state/head_battery", 1, &JustinaHardware::callbackHeadBattery);

    for(int i=0; i< 7; i++)
    {
        JustinaHardware::leftArmCurrentPose.push_back(0);
        JustinaHardware::rightArmCurrentPose.push_back(0);
    }

    JustinaHardware::is_node_set = true;
    return true;
}

//Methods for operating head
void JustinaHardware::getHeadCurrentPose(float& pan, float& tilt)
{
    pan = JustinaHardware::headPan;
    tilt = JustinaHardware::headTilt;
}

float JustinaHardware::getHeadCurrentPan()
{
    return JustinaHardware::headPan;
}

float JustinaHardware::getHeadCurrentTilt()
{
    return JustinaHardware::headTilt;
}

void JustinaHardware::setHeadGoalPose(float pan, float tilt)
{
    std_msgs::Float32MultiArray msg;
    msg.data.push_back(pan);
    msg.data.push_back(tilt);
    JustinaHardware::pubHeadGoalPose.publish(msg);
}

//Methods for operating the left arm
float JustinaHardware::getLeftArmCurrentGripper()
{
    return JustinaHardware::leftArmCurrentGripper;
}

void JustinaHardware::getLeftArmCurrentPose(std::vector<float>& currentPose)
{
    currentPose = JustinaHardware::leftArmCurrentPose;
}

void JustinaHardware::setLeftArmGoalGripper(float goalGripper)
{
    std_msgs::Float32 msg;
    msg.data = goalGripper;
    JustinaHardware::pubLeftArmGoalGripper.publish(msg);
}

void JustinaHardware::setLeftArmGoalPose(std::vector<float>& goalAngles)
{
    std_msgs::Float32MultiArray msg;
    msg.data = goalAngles;
    JustinaHardware::pubLeftArmGoalPose.publish(msg);
}

void JustinaHardware::setLeftArmGoalPose(float theta0, float theta1, float theta2, float theta3, float theta4, float theta5, float theta6)
{
    std_msgs::Float32MultiArray msg;
    msg.data.push_back(theta0);
    msg.data.push_back(theta1);
    msg.data.push_back(theta2);
    msg.data.push_back(theta3);
    msg.data.push_back(theta4);
    msg.data.push_back(theta5);
    msg.data.push_back(theta6);
    JustinaHardware::pubLeftArmGoalPose.publish(msg);
}

void JustinaHardware::setLeftArmGoalTorqueGrip(float torqueGripper)
{
    std_msgs::Float32 msg;
    msg.data = torqueGripper;
    JustinaHardware::pubLeftArmGoalTorqueGrip.publish(msg);
}

void JustinaHardware::setLeftArmGoalTorque(std::vector<float>& goalTorques)
{
    std_msgs::Float32MultiArray msg;
    msg.data = goalTorques;
    JustinaHardware::pubLeftArmGoalTorque.publish(msg);
}

void JustinaHardware::setLeftArmGoalTorque(float t0, float t1, float t2, float t3, float t4, float t5, float t6)
{
    std_msgs::Float32MultiArray msg;
    msg.data.push_back(t0);
    msg.data.push_back(t1);
    msg.data.push_back(t2);
    msg.data.push_back(t3);
    msg.data.push_back(t4);
    msg.data.push_back(t5);
    msg.data.push_back(t6);
    JustinaHardware::pubLeftArmGoalTorque.publish(msg);
}

//Methods for operating right arm
float JustinaHardware::getRightArmCurrentGripper()
{
    return JustinaHardware::rightArmCurrentGripper;
}

void JustinaHardware::getRightArmCurrentPose(std::vector<float>& currentPose)
{
    currentPose = JustinaHardware::rightArmCurrentPose;
}

void JustinaHardware::setRightArmGoalGripper(float goalGripper)
{
    std_msgs::Float32 msg;
    msg.data = goalGripper;
    JustinaHardware::pubRightArmGoalGripper.publish(msg);
}

void JustinaHardware::setRightArmGoalPose(std::vector<float>& goalAngles)
{
    std_msgs::Float32MultiArray msg;
    msg.data = goalAngles;
    JustinaHardware::pubRightArmGoalPose.publish(msg);
}

void JustinaHardware::setRightArmGoalPose(float theta0, float theta1, float theta2, float theta3, float theta4, float theta5, float theta6)
{
    std_msgs::Float32MultiArray msg;
    msg.data.push_back(theta0);
    msg.data.push_back(theta1);
    msg.data.push_back(theta2);
    msg.data.push_back(theta3);
    msg.data.push_back(theta4);
    msg.data.push_back(theta5);
    msg.data.push_back(theta6);
    JustinaHardware::pubRightArmGoalPose.publish(msg);
}

void JustinaHardware::setRightArmGoalTorqueGrip(float torqueGripper)
{
    std_msgs::Float32 msg;
    msg.data = torqueGripper;
    JustinaHardware::pubRightArmGoalTorqueGrip.publish(msg);
}

void JustinaHardware::setRightArmGoalTorque(std::vector<float>& goalTorques)
{
    std_msgs::Float32MultiArray msg;
    msg.data = goalTorques;
    JustinaHardware::pubRightArmGoalTorque.publish(msg);
}

void JustinaHardware::setRightArmGoalTorque(float t0, float t1, float t2, float t3, float t4, float t5, float t6)
{
    std_msgs::Float32MultiArray msg;
    msg.data.push_back(t0);
    msg.data.push_back(t1);
    msg.data.push_back(t2);
    msg.data.push_back(t3);
    msg.data.push_back(t4);
    msg.data.push_back(t5);
    msg.data.push_back(t6);
    JustinaHardware::pubRightArmGoalTorque.publish(msg);
}

//Methods for operating the mobile base
void JustinaHardware::setBaseSpeeds(float leftSpeed, float rightSpeed)
{
    std_msgs::Float32MultiArray msg;
    msg.data.push_back(leftSpeed);
    msg.data.push_back(rightSpeed);
    JustinaHardware::pubBaseSpeeds.publish(msg);
}

void JustinaHardware::setBaseCmdVel(float linearX, float linearY, float angular)
{
    geometry_msgs::Twist msg;
    msg.linear.x = linearX;
    msg.linear.y = linearY;
    msg.linear.z = 0;
    msg.angular.x = 0;
    msg.angular.y = 0;
    msg.angular.z = angular;
    JustinaHardware::pubBaseCmdVel.publish(msg);
}

//Methods for operating robot state
float JustinaHardware::getBaseBattery()
{
    return JustinaHardware::baseBattery;
}

float JustinaHardware::getLeftArmBattery()
{
    return JustinaHardware::leftArmBattery;
}

float JustinaHardware::getRightArmBattery()
{
    return JustinaHardware::rightArmBattery;
}

float JustinaHardware::getHeadBattery()
{
    return JustinaHardware::headBattery;
}

//callbacks for head operation
void JustinaHardware::callbackHeadCurrentPose(const std_msgs::Float32MultiArray::ConstPtr& msg)
{
    JustinaHardware::headPan = msg->data[0];
    JustinaHardware::headTilt = msg->data[1];
}

//callbacks for left arm operation
void JustinaHardware::callbackLeftArmCurrentGripper(const std_msgs::Float32::ConstPtr& msg)
{
    JustinaHardware::leftArmCurrentGripper = msg->data;
}

void JustinaHardware::callbackLeftArmCurrentPose(const std_msgs::Float32MultiArray::ConstPtr& msg)
{
    if(msg->data.size() != 7)
    {
        std::cout << "JustinaHardware.->Error in callback for left arm current pose: msg must have 7 values" << std::endl;
        return;
    }
    for(int i=0; i<7; i++)
        JustinaHardware::leftArmCurrentPose[i] = msg->data[i];
}

//callbacks for right arm operation
void JustinaHardware::callbackRightArmCurrentGripper(const std_msgs::Float32::ConstPtr& msg)
{
    JustinaHardware::rightArmCurrentGripper = msg->data;
}

void JustinaHardware::callbackRightArmCurrentPose(const std_msgs::Float32MultiArray::ConstPtr& msg)
{
    if(msg->data.size() != 7)
    {
        std::cout << "JustinaHardware.->Error in callback for right arm current pose: msg must have 7 values" << std::endl;
        return;
    }
    for(int i=0; i<7; i++)
        JustinaHardware::rightArmCurrentPose[i] = msg->data[i];
}

//callbacks for robot state
void JustinaHardware::callbackBaseBattery(const std_msgs::Float32::ConstPtr& msg)
{
    JustinaHardware::baseBattery = msg->data;
}

void JustinaHardware::callbackLeftArmBattery(const std_msgs::Float32::ConstPtr& msg)
{
    JustinaHardware::leftArmBattery = msg->data;
}

void JustinaHardware::callbackRightArmBattery(const std_msgs::Float32::ConstPtr& msg)
{
    JustinaHardware::rightArmBattery = msg->data;
}

void JustinaHardware::callbackHeadBattery(const std_msgs::Float32::ConstPtr& msg)
{
    JustinaHardware::headBattery = msg->data;
}
