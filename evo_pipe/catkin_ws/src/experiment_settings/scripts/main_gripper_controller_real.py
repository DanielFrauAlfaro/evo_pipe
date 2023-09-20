#!/usr/bin/env python3

import rospy
import sys
from std_msgs.msg import Float64, Float32
from robotiq_3f_gripper_articulated_msgs.msg import Robotiq3FGripperRobotOutput, Robotiq3FGripperRobotInput

# Messages
msg_3f = Robotiq3FGripperRobotOutput()
msg_3f.rACT = 1
msg_3f.rGTO = 1

# Publishers commands
pub_cmd = []

# States list
pub_state = []


# Start the node
start_node = False

# Position command callback
def cmd_cb(data):
    global pub_cmd, msg_3f
    
    msg = []

    
    msg_3f.rPRA = int(data.data)
    msg.append(msg_3f)


    # Publishes the message
    pub_cmd[0].publish(msg[0])



# Output es el cmd

# ---- Main ----
if __name__ == "__main__":

    # Node
    rospy.init_node("gripper_controller")

    # --- Publishers ---
    # Publisher of the gripper state
    # pub_state.append(rospy.Publisher("/" + name + "/grip_state", Float32, queue_size=10))

    # --- Subscribers ---
    # Topic for the gripper commands
    rospy.Subscriber("/aurova/grip_cmd", Float64, cmd_cb)

    # --- Publishers ---
    pub_cmd.append(rospy.Publisher("/Robotiq3FGripperRobotOutput", Robotiq3FGripperRobotOutput, queue_size=10))
    

    
    # Spin
    rospy.spin()