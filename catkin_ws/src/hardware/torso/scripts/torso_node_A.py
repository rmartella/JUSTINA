#!/usr/bin/env python
#possible improve: take out sensor outlayer in the arduino code
import serial, time, sys, math
import rospy
import os
import comm
from datetime import datetime
from std_msgs.msg import Float32
from std_msgs.msg import Float32MultiArray
from std_msgs.msg import Bool
from std_msgs.msg import Empty
from std_msgs.msg import String
from sensor_msgs.msg import JointState


THR_DIFF_POS       = 2
MSG_MOTOR_TIMEOUT  = 1000000 #delay in microseconds 
MSG_SENSOR_TIMEOUT = 500000  #delay in microseconds 
DIST_LIM_INF       = 20.0
DIST_LIM_SUP       = 50.0


def printHelp():
    print "Torso. Options:"
    print "\t --port \t    Serial port name. If not provided, the default value is \"/dev/justinaTorso\""
    print "\t --simul\t    Simulation mode."
    print "PLEASE DON'T TRY TO OPERATE JUSTINA IF YOU ARE NOT QUALIFIED ENOUGH."

def callbackStop(msg):
    global stop
    stop = True

#169733------0.352

def callbackRelative(msg):
    global relH
    global stop
    global valueRel
    valueRel = True
    relH = msg.data[0]*100 ##Pasar de metros a pulsos
    stop = False

def callbackAbsolute(msg):
    global absH
    global stop
    global valueAbs
    valueAbs = True
    absH = msg.data[0]*100 ##Pasar de metros a pulsos
    stop = False 

def callbackTorsoUp(msg):
    global torsoUp 
    global stop
    torsoUp = True
    stop = False 

def callbackTorsoDown(msg):
    global torsoDown 
    global stop
    torsoDown = True 
    stop = False 

def main(portName1, simulated):
    print "INITIALIZING TORSO..."


    ###Connection with ROS
    rospy.init_node("torso")

    jointStates = JointState()
    jointStates.name = ["spine_connect","waist_connect","shoulders_connect", "shoulders_left_connect", "shoulders_right_connect"]
    jointStates.position = [0.0, 0.0, 0.0, 0.0, 0.0]

    pubTorsoPos       = rospy.Publisher ("/hardware/torso/current_pose",Float32MultiArray, queue_size = 1) 
    pubGoalReached    = rospy.Publisher ("/hardware/torso/goal_reached", Bool, queue_size=1)
    pubJointStates    = rospy.Publisher ("/joint_states", JointState, queue_size = 1)
    subRelativeHeight = rospy.Subscriber("/hardware/torso/goal_rel_pose",Float32MultiArray, callbackRelative)
    subAbsoluteHeight = rospy.Subscriber("/hardware/torso/goal_pose",Float32MultiArray, callbackAbsolute)
    subStop           = rospy.Subscriber("robot_state/stop", Empty, callbackStop)
    subTorsoUp        = rospy.Subscriber("/hardware/torso/torso_up",String, callbackTorsoUp)
    subTorsoDown      = rospy.Subscriber("/hardware/torso/torso_down",String, callbackTorsoDown)
    
    rate = rospy.Rate(30)
    ###Communication with the Roboclaw
    global valueRel
    global valueAbs
    global absH
    global relH
    global stop
    global torsoUp
    global torsoDown

    valueAbs  = False
    valueRel  = False
    torsoUp   = False
    torsoDown = False
    torsoPos  = 0
    bumper    = 0
    msgCurrentPose = Float32MultiArray()
    msgGoalReached = Bool()
    msgCurrentPose.data = [0,0,0]
    msgMotor = None
    initTimeMtrMsg = datetime.now()
    initTimeSnrMsg = datetime.now()
    timeoutSnr = 0
    timeoutMtr = 0
    ArdIfc = comm.Comm(portName1)
    msgSensor = comm.Msg(comm.ARDUINO_ID, comm.MOD_SENSORS, comm.OP_GETCURRENTDIST, [], 0)
    goalPose = 0
    while not rospy.is_shutdown():
        try:
            ArdIfc.send(msgSensor)
            timeoutSnr = datetime.now() - initTimeSnrMsg
            if timeoutSnr.microseconds > MSG_SENSOR_TIMEOUT:
                ArdIfc.send(msgSensor)
            initTimeSnrMsg = datetime.now()
            newMsg = ArdIfc.recv()
            if newMsg != None:
                if newMsg.mod == comm.MOD_SENSORS: 
                    if newMsg.op == comm.OP_GETCURRENTDIST:
                        torsoPos = newMsg.param[0]
                if newMsg.mod == comm.MOD_SYSTEM: 
                    if newMsg.op == comm.OP_PING:
                        rospy.loginfo("Torso-> Arduino ack PING msg received.")
                if newMsg.mod == comm.MOD_MOTORS:
                    if newMsg.op == comm.OP_SETTORSOPOSE:
                        msgMotor_ack_received = True
                        rospy.loginfo("Torso-> Arduino ack SETTORSOPOSE msg received.")
                    if newMsg.op == comm.OP_GOUP:
                        msgMotor_ack_received = True
                        rospy.loginfo("Torso-> Arduino ack GOUP msg received.")
                    if newMsg.op == comm.OP_GODOWN:
                        msgMotor_ack_received = True
                        rospy.loginfo("Torso-> Arduino ack GODOWN msg received.")

            #until ack received
            timeoutMtr = datetime.now() - initTimeMtrMsg
            if msgMotor != None and timeoutMtr.microseconds > MSG_MOTOR_TIMEOUT and not msgMotor_ack_received:
                 ArdIfc.send(msgMotor)

            
            if valueAbs and  not stop and absH >= DIST_LIM_INF and absH <= DIST_LIM_SUP:
                msgMotor_ack_received = False
                msgMotor = comm.Msg(comm.ARDUINO_ID, comm.MOD_MOTORS, comm.OP_SETTORSOPOSE, int(absH), 1)
                ArdIfc.send(msgMotor)
                valueAbs=False
                goalPose= absH
                initTimeMtrMsg = datetime.now()
            elif valueRel and not stop and (torsoPos + relH) >= DIST_LIM_INF and (torsoPos + relH) <= DIST_LIM_SUP:
                msgMotor_ack_received = False
                absCalH = torsoPos + relH
                msgMotor = comm.Msg(comm.ARDUINO_ID, comm.MOD_MOTORS, comm.OP_SETTORSOPOSE, int(absCalH), 1)
                ArdIfc.send(msgMotor)
                goalPose=absCalH
                valueRel = False
                initTimeMtrMsg = datetime.now()
            elif ( valueAbs and (absH <  DIST_LIM_INF or absH > DIST_LIM_SUP) ) or ( valueRel and (torsoPos+relH > DIST_LIM_SUP or torsoPos+relH < DIST_LIM_INF) ):
                rospy.logerr("Torso-> Can not reach te position.")
                valueAbs = False
                valueRel = False
                goalPose= torsoPos
            elif torsoUp and not stop:
                rospy.loginfo("Torso-> Moving torso up.")
                msgMotor = comm.Msg(comm.ARDUINO_ID, comm.MOD_MOTORS, comm.OP_GOUP, [], 0)
                ArdIfc.send(msgMotor)
                torsoUp = False
                msgMotor_ack_received = False
                initTimeMtrMsg = datetime.now()
                goalPose= torsoPos
            elif torsoDown and not stop:
                rospy.loginfo("Torso-> Moving torso down.")
                msgMotor = comm.Msg(comm.ARDUINO_ID, comm.MOD_MOTORS, comm.OP_GODOWN, [], 0)
                ArdIfc.send(msgMotor)
                torsoDown = False
                msgMotor_ack_received = False 
                initTimeMtrMsg = datetime.now()
                goalPose= torsoPos

            
            jointStates.header.stamp = rospy.Time.now()
            jointStates.position = [torsoPos, 0.0, 0.0, 0.0, 0.0]
            pubJointStates.publish(jointStates)
            
            msgCurrentPose.data[0] = torsoPos/100.0
            msgCurrentPose.data[1] = 0.0
            msgCurrentPose.data[2] = 0.0
            pubTorsoPos.publish(msgCurrentPose)
            msgGoalReached.data = abs(goalPose - torsoPos) < THR_DIFF_POS 
            pubGoalReached.publish(msgGoalReached)

            rate.sleep()
        except:
            rospy.logerr("Torso-> Oopps...we have some issues in this node :( ")    

    #End of while

#end of main()

if __name__ == '__main__':
    try:
        if "--help" in sys.argv:
            printHelp()
        elif "-h" in sys.argv:
            printHelp()
        else:
            portName1 = "/dev/justinaTorso"
            simulated = False
            if "--port" in sys.argv:
                portName1 = sys.argv[sys.argv.index("--port1") + 1]
            if "--simul" in sys.argv:
                simulated = True
            main(portName1, simulated)
    except rospy.ROSInterruptException:
        pass
