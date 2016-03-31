#include <iostream>
#include <cmath>
#include "ros/ros.h"
#include "geometry_msgs/PoseWithCovarianceStamped.h"
#include "geometry_msgs/Pose2D.h"
#include "std_msgs/Float32.h"
#include "std_msgs/Float32MultiArray.h"
#include "std_msgs/Bool.h"
#include "LowLevelControl.h"

class SimpleMoveNode
{
public:
    SimpleMoveNode();
    ~SimpleMoveNode();

    ros::NodeHandle nh;
    ros::Publisher pubGoalReached;
    ros::Publisher pubSpeeds;
    ros::Subscriber subCurrentPose;
    ros::Subscriber subGoalPose;
    ros::Subscriber subGoalDistance;
    ros::Subscriber subGoalRelativePose;

    LowLevelControl control;
    float goalX;
    float goalY;
    float goalTheta;
    float currentX;
    float currentY;
    float currentTheta;
    bool newGoal;
    bool moveBackwards;

    void initROSConnection();
    void spin();

private:
    void callbackGoalPose(const geometry_msgs::Pose2D::ConstPtr& msg);
    void callbackGoalDist(const std_msgs::Float32::ConstPtr& msg);
    void callbackGoalRelPose(const geometry_msgs::Pose2D::ConstPtr& msg);
    void callbackCurrentPose(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr& msg);
};