#! /usr/bin/python3

# from spatialmath.base import symbolic as sym
#import spatialmath
from roboticstoolbox import DHRobot, RevoluteDH
import rospy
from geometry_msgs.msg import Pose
from math import pi
import actionlib
from control_msgs.msg import FollowJointTrajectoryAction
from trajectory_msgs.msg import JointTrajectoryPoint
from control_msgs.msg import FollowJointTrajectoryGoal
from std_msgs.msg import Duration
import experiment_settings.msg
from tf.transformations import euler_from_quaternion, quaternion_from_euler


class IKAction(object):
    _feedback = experiment_settings.msg.IKFeedback()
    _result = experiment_settings.msg.IKResult()

    def __init__(self, name):
        self._action_name = name
        self._as = actionlib.SimpleActionServer(self._action_name, experiment_settings.msg.IKAction, execute_cb=self.execute_cb, auto_start = False)

        self.ur5 = DHRobot([
            RevoluteDH(d=0.1625, alpha=pi/2.0),
            RevoluteDH(a=-0.425),
            RevoluteDH(a = -0.3922),
            RevoluteDH(d = 0.1333, alpha=pi/2.0),
            RevoluteDH(d = 0.0997, alpha=-pi/2.0),
            RevoluteDH(d = 0.0996)
        ], name="UR5e")

        self.client = actionlib.SimpleActionClient("ur5_2/trajectory_controller/follow_joint_trajectory", FollowJointTrajectoryAction)
        self.goal = FollowJointTrajectoryGoal()
        self.goal.trajectory.joint_names = ["shoulder_pan_joint", "shoulder_lift_joint", "elbow_joint",
                                           "wrist_1_joint", "wrist_2_joint", "wrist_3_joint"]
        p = [JointTrajectoryPoint()]
        p[0].positions = [-pi, -pi/2.0, -pi/2.0, -pi/2.0, pi/2.0, pi/4.0]
        p[0].velocities = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]

        p[0].time_from_start = rospy.Duration(2.5)

        self.goal.trajectory.points = p

        self._as.start()


    def execute_cb(self,goal):
        print("Goal Recieved")

        x = goal.pose.position.x                                 
        y = goal.pose.position.y
        z = goal.pose.position.z

        orientation_list = [goal.pose.orientation.x, goal.pose.orientation.y, goal.pose.orientation.z, goal.pose.orientation.w]

        (roll, pitch, yaw) = euler_from_quaternion(orientation_list)                

        self._result.finish = True
        self._as.set_succeeded(self._result)

    

if __name__ == "__main__":
    rospy.init_node('ik_server_node')
    server = IKAction("ik_server")
    rospy.spin()