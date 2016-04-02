#include "QtRosNode.h"

QtRosNode::QtRosNode()
{
    this->gui_closed = false;
}

QtRosNode::~QtRosNode()
{
}

void QtRosNode::run()
{
    this->n = new ros::NodeHandle();
    this->pub_SimpleMove_GoalDist = this->n->advertise<std_msgs::Float32>("/navigation/path_planning/simple_move/goal_dist", 1);
    ros::Subscriber subCurrentPose = this->n->subscribe("/navigation/localization/current_pose", 1, &QtRosNode::callbackCurrentPose, this);
    
    ros::Rate loop(10);
    while(ros::ok() && !this->gui_closed)
    {
        //std::cout << "Ros node running..." << std::endl;
        ros::spinOnce();
        loop.sleep();
    }
    emit onRosNodeFinished();
}

void QtRosNode::publish_SimpleMove_GoalDist(float goalDist)
{
    std_msgs::Float32 msgDist;
    msgDist.data = goalDist;
    this->pub_SimpleMove_GoalDist.publish(msgDist);
    ros::spinOnce();
}

void QtRosNode::publish_PathCalculator_WaveFront(float currentX, float currentY, float currentTheta, float goalX, float goalY, float goalTheta)
{
    nav_msgs::GetMap srvGetMap;
    navig_msgs::PathFromMap srvPathFromMap;
    ros::ServiceClient srvCltGetMap = this->n->serviceClient<nav_msgs::GetMap>("/navigation/localization/static_map");
    ros::ServiceClient srvCltPathFromMap = this->n->serviceClient<navig_msgs::PathFromMap>("/navigation/path_planning/path_calculator/wave_front");
    srvCltGetMap.call(srvGetMap);
    ros::spinOnce();
    srvPathFromMap.request.map = srvGetMap.response.map;
    srvPathFromMap.request.start_pose.position.x = currentX;
    srvPathFromMap.request.start_pose.position.y = currentY;
    srvPathFromMap.request.start_pose.orientation.w = cos(currentTheta/2);
    srvPathFromMap.request.start_pose.orientation.z = sin(currentTheta/2);
    srvPathFromMap.request.goal_pose.position.x = goalX;
    srvPathFromMap.request.goal_pose.position.y = goalY;
    srvPathFromMap.request.goal_pose.orientation.w = cos(goalTheta/2);
    srvPathFromMap.request.goal_pose.orientation.z = sin(goalTheta/2);
    if(srvCltPathFromMap.call(srvPathFromMap))
        std::cout << "QtRosNode.->Path calculated succesfully by path_calculator node" << std::endl;
    else
        std::cout << "QtRosNode.->Cannot calculate path by path_calculator node" << std::endl;
    ros::spinOnce();
}

void QtRosNode::callbackCurrentPose(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr& msg)
{
    float currentX = msg->pose.pose.position.x;
    float currentY = msg->pose.pose.position.y;
    float currentTheta = atan2(msg->pose.pose.orientation.z, msg->pose.pose.orientation.w) * 2;
    //std::cout << "JustinaGUI.->Current pose: " << currentX << "  " << currentY << "  " << currentTheta << std::endl;
    emit onCurrentPoseReceived(currentX, currentY, currentTheta);
}
