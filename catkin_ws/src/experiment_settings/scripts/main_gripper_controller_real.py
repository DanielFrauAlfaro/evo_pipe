#!/usr/bin/env python3

import rospy
import numpy as np
import sys
from std_msgs.msg import Int32, Int16MultiArray
from robotiq_3f_gripper_articulated_msgs.msg import Robotiq3FGripperRobotOutput, Robotiq3FGripperRobotInput

# Messages
msg_3f = Robotiq3FGripperRobotOutput()
msg_3f.rACT = 1
msg_3f.rGTO = 1

# Publishers commands
pub_cmd = []


# gDTA -> Contacto con el dedo A
# gDTC -> Contacto con el dedo C
# El valor de gDTB no indica el contacto, por lo que no se debería usar


def input_cb(data):
    global pub_cmd

    msg = Int16MultiArray()

    msg.data = np.array([0, 0])

    if data.gDTA == 1:
        msg.data[0] = 1
    if data.gDTC == 1:
        msg.data[1] == 1

    pub_cmd[1].publish(msg)
    


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

    # --- Subscribers ---
    # Topic for the gripper commands
    rospy.Subscriber("/aurova/grip_cmd", Int32, cmd_cb)
    rospy.Subscriber("/Robotiq3FGripperRobotInput", Robotiq3FGripperRobotInput, input_cb)

    # --- Publishers ---
    pub_cmd.append(rospy.Publisher("/Robotiq3FGripperRobotOutput", Robotiq3FGripperRobotOutput, queue_size=10))
    pub_cmd.append(rospy.Publisher("/aurova/contacts_gripper", Int16MultiArray, queue_size=10))

    
    # Spin
    rospy.spin()