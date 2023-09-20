//standard
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <math.h>
#include <cmath>
#include <Eigen/Geometry>
#include <fstream>
#include <math.h> 

// ROS
#include <ros/ros.h>
#include <ros/topic.h>
#include <tf/transform_listener.h>
#include <tf_conversions/tf_eigen.h>
#include <rosbag/bag.h>
#include "rosbag/view.h"

// MoveIt!
#include <moveit/move_group_interface/move_group_interface.h>
#include <moveit/planning_scene_interface/planning_scene_interface.h>
#include <rviz_visual_tools/rviz_visual_tools.h>
#include <moveit_msgs/DisplayRobotState.h>
#include <moveit_msgs/DisplayTrajectory.h>

//messages type
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Float32MultiArray.h"
#include "std_msgs/Float64.h"
#include "std_msgs/Int64.h"
#include "robotiq_2f_gripper_control/Robotiq2FGripper_robot_output.h"
#include <yolact_ros/Result.h>
#include <moveit_msgs/RobotTrajectory.h>


// CONST VARIABLES
const std::string CAMERA_FRAME = "/camera_color_optical_frame";

#define PI 3.14159265

// Segun en que se trabaje, se debe comentar uno o otro. CUIDADO.
// Robot real
const std::string WORLD_FRAME = "/base";
//Robot en simulacion
//const std::string WORLD_FRAME = "/world";

const std::string ARM_PLANNING_GROUP = "ur5e_arm";
const std::string GRIPPER_PLANNING_GROUP = "gripper";
const std::string ARM_END_EFFECTOR_LINK = "ee_link";

// Ficheros de trayectorias
std::string ruta_carpeta = "/../../home/aurova/aurova_ws/src/ignacio/ur5e_moveit_config_laboratory/trajectories/";
std::string tmp = "/../../home/aurova/aurova_ws/src/ignacio/ur5e_moveit_config_laboratory/src/";

std::string nav_home ="nav_home.bag";
std::string home_nav ="home_nav.bag";
std::string home_pic ="home_picture.bag";
std::string pic_home ="picture_home.bag";


ros::Publisher pub_jointValue;
ros::Publisher pub_end_of_robot_grasping;
ros::Publisher pub_grasping;
ros::Publisher pub_last_point_robot;


Eigen::Vector3d transformPoint(const tf::Stamped<tf::Point> & tfPointIn, const std::string & sourceFrame, const std::string & targetFrame) {
  tf::TransformListener transformer;
  tf::Stamped<tf::Point> tfPointOut;

  transformer.waitForTransform(targetFrame, sourceFrame, ros::Time(0), ros::Duration(3.0));
  transformer.transformPoint(targetFrame, tfPointIn, tfPointOut);

  Eigen::Vector3d outputPoint(tfPointOut.getX(), tfPointOut.getY(), tfPointOut.getZ());

  return outputPoint;
}

Eigen::Vector3d transformVector(const tf::Stamped<tf::Vector3> & tfVectorIn, const std::string & sourceFrame, const std::string & targetFrame) {
  tf::TransformListener transformer;
  tf::Stamped<tf::Vector3> tfVectorOut;

  transformer.waitForTransform(targetFrame, sourceFrame, ros::Time(0), ros::Duration(3.0));
  transformer.transformVector(targetFrame, tfVectorIn, tfVectorOut);

  Eigen::Vector3d outputVector(tfVectorOut.getX(), tfVectorOut.getY(), tfVectorOut.getZ());

  return outputVector;
}

//recibe la distancia entre puntos de agarre y devuelve el valor articular destino de la pinza en radianes (0.0 -> 0.8)
double getValueJointGripper(double width)
{
	double L2 = 0.1, L2x = 0.0127, Lp = 0.00735;
	double offsetTheta = 48 * M_PI / 180.0;
	double cosTheta = (width/2 + Lp - L2x)/L2;
	double result = acos(cosTheta) - offsetTheta;

	if (result < 0.0){
		result = 0.0;
	}
	else if (result > 0.8){
		result = 0.8;
	}
	return result;
}

//recibe el valor articular de la pinza y devuelve la altura entre el efector final del ur y el dedo
double getValueHeightGripper(double angle)
{
	double offsetTheta = 48 * M_PI / 180.0;
	double L2 = 0.1, L2y = 0.06942, Lh = 0.012; //Lh = longitud entre el inicio del dedo y el punto de contacto (longitud a lo largo del dedo)
	return L2y + L2 * sin(angle + offsetTheta) + Lh;
}


// Recibe la ruta de la trayectoria a ejecutar y devuelve un msg con ella cargada
moveit_msgs::RobotTrajectory executeSavedTrajectory (std::string path){

	rosbag::Bag bag;
	moveit_msgs::RobotTrajectory temp_msg;

	bag.open(path, rosbag::bagmode::Read);
	
	for(rosbag::MessageInstance const m : rosbag::View(bag))
	{
		std::string imgTopic = m.getTopic();

		if ("trajectory_msgs/JointTrajectory" == imgTopic){
			temp_msg = *m.instantiate<moveit_msgs::RobotTrajectory>();
		}
	}

	bag.close();

	return temp_msg;
}


void planning_robot()
{
	ros::NodeHandle n;

	rviz_visual_tools::RvizVisualToolsPtr visualTools;
	visualTools.reset(new rviz_visual_tools::RvizVisualTools(WORLD_FRAME, "/rviz_visual_markers"));
	visualTools->trigger();

	moveit::planning_interface::MoveGroupInterface armMoveGroup(ARM_PLANNING_GROUP);
	armMoveGroup.setPlannerId ("RRTstar");
	//armMoveGroup.setPlanningTime(5.0);

	
        /*moveit::planning_interface::MoveGroupInterface gripperMoveGroup(GRIPPER_PLANNING_GROUP);
        const robot_state::JointModelGroup* joint_model_group = gripperMoveGroup.getCurrentState()->getJointModelGroup(GRIPPER_PLANNING_GROUP);*/

	while(1){

		// Go to home pose to start the process
		moveit_msgs::RobotTrajectory msg;				

		std::cout << "EJECUTANDO IDA A HOME" << std::endl;
		msg = executeSavedTrajectory (ruta_carpeta + nav_home);
		armMoveGroup.execute(msg);
		ros::Duration(0.1).sleep();					

		std::cout << "EJECUTANDO IDA A CAM" << std::endl;
		msg = executeSavedTrajectory (ruta_carpeta + home_pic);
		armMoveGroup.execute(msg);


		// get class of manipulated object -> subscribe to topic and process its data later 
		yolact_ros::Result::ConstPtr mrcnn_results_msgs =  ros::topic::waitForMessage<yolact_ros::Result>("/yolact/result", n);

		std::cout << "Waiting for grasping data computed..." << std::endl;
		std_msgs::Float32MultiArray::ConstPtr grasping_data_vector = ros::topic::waitForMessage<std_msgs::Float32MultiArray>("/grasp_data_to_planning", n);

		// ===============================================================================================================
		// PROCESS TO COMPUTE DISTANCES ROBOT-OBJECT (COMPUTING ORDER TO GRASP DIFFERENTS OBJECTS)

		int grasping_data_values_one_object = 24; //each object has 24 grasping data
		int objects_detected = grasping_data_vector->data.size() / grasping_data_values_one_object; 
		std::vector<std::vector<float> > grasping_data_vectors(objects_detected); // vectors filled with each grasping data object
		std::cout << "Objects detected: " << objects_detected << std::endl;


		if (objects_detected > 0){
			

			// create one vector for each object to store correspondent grasping data 
			int i=0;
			int aux_contator = 0;
			for(int j=0; j<grasping_data_vector->data.size(); j++){
				if(aux_contator == grasping_data_values_one_object){
					i++;
					aux_contator = 0;
				}
				grasping_data_vectors[i].push_back(grasping_data_vector->data[j]);
				aux_contator++;
			}

			// ===============================================================================================================
			// GET CLASS OF OBJECTS
		 
			std::vector < int > class_objects_detected;
			int count_objects_detected = mrcnn_results_msgs->class_ids.size(); //counter of objects detected

			for (int detected_object=0; detected_object<count_objects_detected; detected_object++){
				class_objects_detected.push_back(mrcnn_results_msgs->class_ids[i]);
			}

			std::cout << "Since we are treating one object, its class is: " << class_objects_detected [0] << std::endl;

			
			////////////////////////////////
			// Sólo voy a tratar el primero:
			if(objects_detected >1){ 
				objects_detected=1;
				std::cout << "New Objects detected: " << objects_detected << std::endl;
			}
			else{
				std::cout << "Number of objects detected, same as previous." << std::endl;
			}

			/*std::cout<<" "<<std::endl; 
			geometry_msgs::PoseStamped lectura = armMoveGroup.getCurrentPose("ee_link");
			std::cout << "Pose leida: " << lectura << std::endl;


		    	pub_last_point_robot.publish(lectura);

			std::cout<<" "<<std::endl;*/

			// calcule distances between robot and each object
			std::vector<float> distances_robot_object_vector;
			for(int i=0; i<objects_detected; i++){

				tf::Stamped<tf::Point> firstPointIn, secondPointIn;
				
				firstPointIn.setX(grasping_data_vectors[i][0]);
				firstPointIn.setY(grasping_data_vectors[i][1]);
				firstPointIn.setZ(grasping_data_vectors[i][2]);

				firstPointIn.frame_id_ = CAMERA_FRAME;
				secondPointIn.setX(grasping_data_vectors[i][3]);
				secondPointIn.setY(grasping_data_vectors[i][4]);
				secondPointIn.setZ(grasping_data_vectors[i][5]);
				secondPointIn.frame_id_ = CAMERA_FRAME;

				Eigen::Vector3d firstPoint = transformPoint(firstPointIn, CAMERA_FRAME, WORLD_FRAME);
				Eigen::Vector3d secondPoint = transformPoint(secondPointIn, CAMERA_FRAME, WORLD_FRAME);
				// pose of each object respect world frame (respect base of robot)
				Eigen::Vector3d centerPoint = (firstPoint + secondPoint) / 2.0;

				double distance = std::sqrt(std::pow(centerPoint[0], 2) + std::pow(centerPoint[1], 2) + std::pow(centerPoint[2], 2));
				std::cout << "Distancia del robot al objeto " << i+1 << ": " << distance << std::endl;

				distances_robot_object_vector.push_back(distance);
			}
			// order distances
			std::vector<float> distances_robot_object_vector_organized = distances_robot_object_vector;
			float aux;
			for(int i=0; i<distances_robot_object_vector_organized.size(); i++){
				for(int j=0; j<distances_robot_object_vector_organized.size()-1; j++){
					if(distances_robot_object_vector_organized[j] > distances_robot_object_vector_organized[j+1]){
						aux = distances_robot_object_vector_organized[j];
						distances_robot_object_vector_organized[j] = distances_robot_object_vector_organized[j+1];
						distances_robot_object_vector_organized[j+1] = aux;
					}
				}
			}
			// create vector with order to execute movements
			std::vector<int> objects_order_execution_vector; // vector with priority of grasping execution order
			for(int i=0; i<distances_robot_object_vector_organized.size(); i++){
				for(int j=0; j<distances_robot_object_vector_organized.size(); j++){
					if(distances_robot_object_vector_organized[i] == distances_robot_object_vector[j]){
						// j => insert index with more priority
						objects_order_execution_vector.push_back(j);
					}
				}
			}

			// ===============================================================================================================
			std::cout << "Buscando grasping " << std::endl;
			for(int i=0; i<objects_detected; i++){

				/* CAMBIOS Z EN MISMA ALTURA*/
				if (grasping_data_vectors[objects_order_execution_vector[i]][2] > grasping_data_vectors[objects_order_execution_vector[i]][5]){
					grasping_data_vectors[objects_order_execution_vector[i]][2] = grasping_data_vectors[objects_order_execution_vector[i]][5];
				}
				else{
					grasping_data_vectors[objects_order_execution_vector[i]][5] = grasping_data_vectors[objects_order_execution_vector[i]][2];
				}

				tf::Stamped<tf::Point> firstPointIn, secondPointIn;
				firstPointIn.setX(grasping_data_vectors[objects_order_execution_vector[i]][0]-0.05);
				firstPointIn.setY(grasping_data_vectors[objects_order_execution_vector[i]][1]-0.02);
				firstPointIn.setZ(grasping_data_vectors[objects_order_execution_vector[i]][2]+0.0125);
				firstPointIn.frame_id_ = CAMERA_FRAME;
				secondPointIn.setX(grasping_data_vectors[objects_order_execution_vector[i]][3]-0.05);
				secondPointIn.setY(grasping_data_vectors[objects_order_execution_vector[i]][4]-0.02);
				secondPointIn.setZ(grasping_data_vectors[objects_order_execution_vector[i]][5]+0.0125);
				secondPointIn.frame_id_ = CAMERA_FRAME;

				std::cout<<"En z..."<<firstPointIn.getZ()<<std::endl;
				std::cout<<"En z2..."<<secondPointIn.getZ()<<std::endl;

				//Eigen::Vector3d mid_point_object = (firstPointIn + secondPointIn) / 2.0;



				tf::Stamped<tf::Vector3> axeXVectorIn;
				std::cout << "X grasp_x : "<<grasping_data_vectors[objects_order_execution_vector[i]][9] << std::endl;
				std::cout << "X grasp_y : "<<grasping_data_vectors[objects_order_execution_vector[i]][10] << std::endl;
				std::cout << "X grasp_z : "<<grasping_data_vectors[objects_order_execution_vector[i]][11] << std::endl;

				axeXVectorIn.setX(grasping_data_vectors[objects_order_execution_vector[i]][9]);
				axeXVectorIn.setY(grasping_data_vectors[objects_order_execution_vector[i]][10]);
				axeXVectorIn.setZ(grasping_data_vectors[objects_order_execution_vector[i]][11]);

				//axeXVectorIn.setX((firstPointIn[0] + secondPointIn[0]) / 2.0);
				//axeXVectorIn.setY((firstPointIn[1] + secondPointIn[1]) / 2.0);
				//axeXVectorIn.setZ((firstPointIn[2] + secondPointIn[2]) / 2.0);
				axeXVectorIn.frame_id_ = CAMERA_FRAME;

				tf::Stamped<tf::Vector3> axeYVectorIn;
				axeYVectorIn.setX(grasping_data_vectors[objects_order_execution_vector[i]][15]);
				axeYVectorIn.setY(grasping_data_vectors[objects_order_execution_vector[i]][16]);
				axeYVectorIn.setZ(grasping_data_vectors[objects_order_execution_vector[i]][17]);

				std::cout << "Y grasp_x : "<<grasping_data_vectors[objects_order_execution_vector[i]][15] << std::endl;
				std::cout << "Y grasp_y : "<<grasping_data_vectors[objects_order_execution_vector[i]][16] << std::endl;
				std::cout << "Y grasp_z : "<<grasping_data_vectors[objects_order_execution_vector[i]][17] << std::endl;

				//axeYVectorIn.setX(firstPointIn[0]);
				//axeYVectorIn.setY(firstPointIn[1]);
				//axeYVectorIn.setZ(grasping_data_vectors[objects_order_execution_vector[i]][23]);

				axeYVectorIn.frame_id_ = CAMERA_FRAME;

				tf::Stamped<tf::Vector3> axeZVectorIn;
				axeZVectorIn.setX(grasping_data_vectors[objects_order_execution_vector[i]][21]);
				axeZVectorIn.setY(grasping_data_vectors[objects_order_execution_vector[i]][22]);
				axeZVectorIn.setZ(grasping_data_vectors[objects_order_execution_vector[i]][23]);

				std::cout << "Z grasp_x : "<<grasping_data_vectors[objects_order_execution_vector[i]][21] << std::endl;
				std::cout << "Z grasp_y : "<<grasping_data_vectors[objects_order_execution_vector[i]][22] << std::endl;
				std::cout << "Z grasp_z : "<<grasping_data_vectors[objects_order_execution_vector[i]][23] << std::endl;
				
				//axeZVectorIn.setX(firstPointIn[2]);
				//axeZVectorIn.setY(firstPointIn[2]);
				//axeZVectorIn.setZ(grasping_data_vectors[objects_order_execution_vector[i]][23]);


				axeZVectorIn.frame_id_ = CAMERA_FRAME;

				// transform points from camera frame to world frame 
				Eigen::Vector3d firstPoint = transformPoint(firstPointIn, CAMERA_FRAME, WORLD_FRAME);
				Eigen::Vector3d secondPoint = transformPoint(secondPointIn, CAMERA_FRAME, WORLD_FRAME);
				Eigen::Vector3d axeXVector = transformVector(axeXVectorIn, CAMERA_FRAME, WORLD_FRAME);
				Eigen::Vector3d axeYVector = transformVector(axeYVectorIn, CAMERA_FRAME, WORLD_FRAME);
				Eigen::Vector3d axeZVector = transformVector(axeZVectorIn, CAMERA_FRAME, WORLD_FRAME);


				//===================================================================================
				double pointsDistance = std::sqrt(std::pow(firstPoint[0] - secondPoint[0], 2) + std::pow(firstPoint[1] - secondPoint[1], 2) + std::pow(firstPoint[2] - secondPoint[2], 2));
				std::cout << "Distance between grasping points: " << pointsDistance << "\n";
				std::cout << "" << std::endl;

				double jointGripper = getValueJointGripper(pointsDistance);
				std::cout << "Articular value of gripper: " << jointGripper << "\n";
				std::cout << "" << std::endl;

				double altura = getValueHeightGripper(jointGripper);
				std::cout << "Heigh of gripper (end effector - tipLink): " << altura << "\n";
				std::cout << "" << std::endl;
				//=========================================================================

				// MODIFICACIONES
				std::cout<<"First point: "<<firstPointIn[0]<<", "<<firstPointIn[1]<<", "<<firstPointIn[2]<<std::endl;
				std::cout<<"Second point: "<<secondPointIn[0]<<", "<<secondPointIn[1]<<", "<<secondPointIn[2]<<std::endl;
				
				// get mid point of object
				Eigen::Vector3d mid_point_object = (firstPoint + secondPoint) / 2.0;


				std::cout << "Punto medio: "<< mid_point_object[0] <<","<< mid_point_object[1]<<","<< mid_point_object[2]<< std::endl;

				Eigen::Vector3d axeAux1, axeAux2, axeAux3;

				axeAux1 = axeXVector;
				axeAux2 = axeZVector;
				axeAux3 = axeYVector;
				axeZVector = - axeAux1;
				axeXVector = - axeAux2;
				axeYVector = - axeAux3;

			
				// normalize vectors
				axeXVector.normalize();
				axeYVector.normalize();
				axeZVector.normalize();

				///////////////////////// Todo lo que aqui modifique para que solo gire en el angulo que tiene el objeto y no en el resto de los ejes.
				///////////////////////=============================================================================================================
				axeXVector[2] = 0;
				axeYVector[2] = 0;
				axeZVector[0] = 0;
				axeZVector[1] = 0;
				axeZVector[2] = 1;
				axeXVector[0] = 1;
				axeXVector[1] = 0;
				axeYVector[0] = 0;
				axeYVector[1] = 1;



				float ang_z = atan ((secondPoint[1]-firstPoint[1])/(secondPoint[0]-firstPoint[0]));
				std::cout << "ang_z = " << ang_z << std::endl;


				Eigen::Matrix3d rotation_matrix_org;
				rotation_matrix_org << axeXVector[0], axeYVector[0], axeZVector[0], 
						   		       axeXVector[1], axeYVector[1], axeZVector[1],
						               axeXVector[2], axeYVector[2], axeZVector[2];


				 
				
				Eigen::Matrix3d rotation_matrix_x;
				rotation_matrix_x <<    1, 0, 0, 
						   		        0, cos(PI),-sin(PI),
						                0, sin(PI), cos(PI);



										

				Eigen::Matrix3d rotation_matrix_z;
				rotation_matrix_z <<    cos(ang_z), -sin(ang_z), 0, 
						   		        sin(ang_z), cos(ang_z), 0,
						                0, 0, 1;

				Eigen::Matrix3d rota_1;
				Eigen::Matrix3d rota_2;
				rota_1 = rotation_matrix_x *rotation_matrix_org;
				rota_2 = rotation_matrix_z * rota_1;



				axeXVector[0] = rota_2(0);
				axeXVector[1] = rota_2(3);
				axeXVector[2] = rota_2(6);
				axeYVector[0] = rota_2(1);
				axeYVector[1] = rota_2(4);
				axeYVector[2] = rota_2(7);
				axeZVector[0] = rota_2(2);
				axeZVector[1] = rota_2(5);
				axeZVector[2] = rota_2(8);

				std::cout << "rota_all" << rota_2 << std::endl;

				std::cout << "rota_0" << rota_2(0)<< std::endl;
				std::cout << "rota_1" << rota_2(1)<< std::endl;
				std::cout << "rota_2" << rota_2(2)<< std::endl;
				std::cout << "rota_3" << rota_2(3)<< std::endl;
				std::cout << "rota_4" << rota_2(4)<< std::endl;
				std::cout << "rota_5" << rota_2(5)<< std::endl;
				std::cout << "rota_6" << rota_2(6)<< std::endl;
				std::cout << "rota_7" << rota_2(7)<< std::endl;
				std::cout << "rota_8" << rota_2(8)<< std::endl;
				///////////////////////=============================================================================================================
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				// visualize object normalized axis
				visualTools->publishSphere(firstPoint, rviz_visual_tools::RED, rviz_visual_tools::LARGE);
				visualTools->publishSphere(secondPoint, rviz_visual_tools::BLUE, rviz_visual_tools::LARGE);
				visualTools->publishSphere(mid_point_object, rviz_visual_tools::GREEN, rviz_visual_tools::LARGE);
				visualTools->publishLine(mid_point_object, mid_point_object + axeXVector, rviz_visual_tools::RED, rviz_visual_tools::MEDIUM); 
				visualTools->publishLine(mid_point_object, mid_point_object + axeYVector, rviz_visual_tools::GREEN, rviz_visual_tools::MEDIUM); 
				visualTools->publishLine(mid_point_object, mid_point_object + axeZVector, rviz_visual_tools::BLUE, rviz_visual_tools::MEDIUM); 
				visualTools->trigger();

				//=======================================================================================
				// ================== DECIDE WHICH CONTAINER IS THE GOAL FOR EACH CLASS =================
				// Remember: CLASS_NAMES = ['BG', 'opaque_plastic_bottle', 'paperboard_box', 'clear_plastic_bottle', 'drink_can', 'opaque_plastic_container']

				class_objects_detected [0] = 3;
				int contenedor;

				switch (class_objects_detected [0]){
					case 0:
						contenedor = 0;
						break;
					case 1:
						contenedor = 1;
						break;
					case 2:
						contenedor = 2;
						break;
					case 3:
						contenedor = 3;
						break;
					case 4:
						contenedor = 0;
						break;
					case 5:
						contenedor = 1;
						break;
					case 6:
						contenedor = 2;
						break;
				}
				
				std::string fase1;
				std::string fase2;
				std::string fase3;
				std::string fase4;		


				if (contenedor == 0){

					fase1 = "home_contDerArrPre.bag";
					fase2 = "contDerArrPre_contDerArrAc.bag";
					fase3 = "contDerArrAc_contDerArrPos.bag";
					fase4 = "contDerArrPos_home.bag";

				}
				else if (contenedor == 1){

					fase1 = "home_contIzqArrPre.bag";
					fase2 = "contIzqArrPre_contIzqArrAc.bag";
					fase3 = "contIzqArrAc_contIzqArrPos.bag";
					fase4 = "contIzqArrPos_home.bag";

				}
				else if (contenedor == 2){

					fase1 = "home_contDerAbPre.bag";
					fase2 = "contDerAbPre_contDerAbAc.bag";
					fase3 = "contDerAbAc_contDerAbPos.bag";
					fase4 = "contDerAbPos_home.bag";

				}
				else{

					fase1 = "home_contIzqAbPre.bag";
					fase2 = "contIzqAbPre_contIzqAbAc.bag";
					fase3 = "contIzqAbAc_contIzqAbPos.bag";
					fase4 = "contIzqAbPos_home.bag";

				}


				//=======================================================================================
				// Vectors and strange stuff (for plotting in rviz i think)

				tf::Stamped<tf::Vector3> axeXp;
				axeXp.setX(1);
				axeXp.setY(0);
				axeXp.setZ(0);
				axeXp.frame_id_ = WORLD_FRAME;
				Eigen::Vector3d axeXVectorPlace(axeXp.getX(), axeXp.getY(), axeXp.getZ());

				tf::Stamped<tf::Vector3> axeYp;
				axeYp.setX(0);
				axeYp.setY(1);
				axeYp.setZ(0);
				axeYp.frame_id_ = WORLD_FRAME;
				Eigen::Vector3d axeYVectorPlace(axeYp.getX(), axeYp.getY(), axeYp.getZ());

				tf::Stamped<tf::Vector3> axeZp;
				axeZp.setX(0);
				axeZp.setY(0);
				axeZp.setZ(1);
				axeZp.frame_id_ = WORLD_FRAME;
				Eigen::Vector3d axeZVectorPlace(axeZp.getX(), axeZp.getY(), axeZp.getZ());

				Eigen::Vector3d axeAux4, axeAux5;
				axeAux4 = axeYVectorPlace;
				axeAux5 = axeZVectorPlace;
				axeYVectorPlace = -axeAux4;
				axeZVectorPlace = -axeAux5;

				// ======================== INIT POINT (RETURN) =======================

				tf::Stamped<tf::Point> init_point_return;
				init_point_return.setX(0.5);
				init_point_return.setY(0.2);
				init_point_return.setZ(0.6);
				init_point_return.frame_id_ = WORLD_FRAME;
				Eigen::Vector3d init_point_returnPoint(init_point_return.getX(), init_point_return.getY(), init_point_return.getZ());

				// ======================== PICTURE POINT (RETURN) =======================

				tf::Stamped<tf::Point> pic_point_return;
				pic_point_return.setX(0.500699);
				pic_point_return.setY(0.00159374);
				pic_point_return.setZ(0.200289);
				pic_point_return.frame_id_ = WORLD_FRAME;
				Eigen::Vector3d pic_point_returnPoint(pic_point_return.getX(), pic_point_return.getY(), pic_point_return.getZ());


				// ======================== NAVEGATION POINT =======================

				geometry_msgs::Pose nav_point;
				nav_point.position.x = -0.133927;
				nav_point.position.y = 0.088266;
				nav_point.position.z = 0.253132;
				nav_point.orientation.x = -0.379784;
				nav_point.orientation.y = 0.380114;
				nav_point.orientation.z = 0.596109;
				nav_point.orientation.w = 0.5966;


				// ========================================================================
				// ========================================================================


				// ========================GRASPING POSE=================================== 
				
				/*Eigen::Vector3d vector_centroid, axeZt_g, vector_v, vector_traslation;
				vector_centroid = mid_point_object - Eigen::Vector3d(0,0,0);
				
				Eigen::Isometry3d grasping_pose;
				Eigen::Matrix3d rotation_matrix;

				Eigen::Matrix3d turn_z;
				turn_z << 1, 0, 0,
					   0,1, 0,
					   0, 0, 1;				

				
				rotation_matrix << axeXVector[0], axeYVector[0], axeZVector[0], 
						   axeXVector[1], axeYVector[1], axeZVector[1],
						   axeXVector[2], axeYVector[2], axeZVector[2];
			

				Eigen::Matrix3d mult;
				mult =   rotation_matrix*turn_z;

				Eigen::Vector3d new_z;
				new_z = Eigen::Vector3d(mult(2),mult(5),mult(8));

rotation_matrix
				grasping_pose.linear() = mult;

				 
				visualTools->publishAxis(grasping_pose, rviz_visual_tools::SMALL);
				visualTools->trigger();*/


				Eigen::Vector3d vector_centroid, axeZt_g, vector_v, vector_traslation;
				vector_centroid = mid_point_object - Eigen::Vector3d(0.0,0.0,0.0);
				axeZt_g = axeZVector * altura;
				vector_v = -axeZt_g;
				vector_traslation = vector_centroid + vector_v;

				Eigen::Isometry3d grasping_pose;
				Eigen::Matrix3d rotation_matrix;
				rotation_matrix << axeXVector[0], axeYVector[0], axeZVector[0], 
						   		   axeXVector[1], axeYVector[1], axeZVector[1],
						           axeXVector[2], axeYVector[2], axeZVector[2];

				grasping_pose.translation() = vector_traslation; 
				grasping_pose.linear() = rotation_matrix;
		 
				visualTools->publishAxis(grasping_pose, rviz_visual_tools::MEDIUM);
				visualTools->trigger();


				// =====================PRE-GRASPING POSE====================================
				
				Eigen::Vector3d axeZt_g2, vector_v2, vector_traslation2;


				axeZt_g2 =  axeZVector * (altura + 0.10);
				vector_v2 = -axeZt_g2;
				vector_traslation2 = vector_centroid + vector_v2;
				
				Eigen::Isometry3d pre_grasping_pose;
				pre_grasping_pose.translation() = vector_traslation2; 
				pre_grasping_pose.linear() = rotation_matrix;

				visualTools->publishAxis(pre_grasping_pose, rviz_visual_tools::LARGE);
				visualTools->trigger();

/*				Eigen::Vector3d axeZt_g2, vector_v2, vector_traslation2;
				axeZt_g2 = axeZVector * (altura + 0.20);
				vector_v2 = -axeZt_g2;
				vector_traslation2 = vector_centroid + vector_v2;
				
				Eigen::Isometry3d pre_grasping_pose;
				pre_grasping_pose.translation() = vector_traslation2; 
				pre_grasping_pose.linear() = rotation_matrix;

				visualTools->publishAxis(pre_grasping_pose, rviz_visual_tools::MEDIUM);
				visualTools->trigger();*/


				// ===================== HOME POSE (RETURN) ====================================

				Eigen::Isometry3d init_point_returnPoint_pose;
				Eigen::Matrix3d init_rotation_matrix_p;
				init_rotation_matrix_p << axeXVectorPlace[0], axeYVectorPlace[0], axeZVectorPlace[0], 
							  axeXVectorPlace[1], axeYVectorPlace[1], axeZVectorPlace[1],
							  axeXVectorPlace[2], axeYVectorPlace[2], axeZVectorPlace[2];
				Eigen::Matrix3d new_rot;
				new_rot << 0,  1, 0,
					  -1,  0, 0,
					   0,  0, 1;
				init_rotation_matrix_p = new_rot * init_rotation_matrix_p;
				init_point_returnPoint_pose.translation() = init_point_returnPoint; 
				init_point_returnPoint_pose.linear() = init_rotation_matrix_p;

				visualTools->publishAxis(init_point_returnPoint_pose, rviz_visual_tools::MEDIUM);
				visualTools->trigger();

				// ===================== PICTURE POSE (RETURN) ====================================

				Eigen::Isometry3d pic_point_returnPoint_pose;
				init_rotation_matrix_p << axeXVectorPlace[0], axeYVectorPlace[0], axeZVectorPlace[0], 
							  axeXVectorPlace[1], axeYVectorPlace[1], axeZVectorPlace[1],
							  axeXVectorPlace[2], axeYVectorPlace[2], axeZVectorPlace[2];

				init_rotation_matrix_p = new_rot * init_rotation_matrix_p;
				pic_point_returnPoint_pose.translation() = pic_point_returnPoint; 
				pic_point_returnPoint_pose.linear() = init_rotation_matrix_p;

				visualTools->publishAxis(pic_point_returnPoint_pose, rviz_visual_tools::MEDIUM);
				visualTools->trigger();


				//===========================================================================
				// PLANNING MOVEMENT - OBJECT MANIPULATION

				moveit::planning_interface::PlanningSceneInterface planningSceneInterface;
				moveit::planning_interface::MoveGroupInterface::Plan armPlan;
				moveit::planning_interface::MoveGroupInterface::Plan gripperPlan;


				char inputChar = 'n';
				moveit_msgs::RobotTrajectory msg;


				
				/*std::cout << "EJECUTANDO IDA A HOME" << std::endl;
				msg = executeSavedTrajectory (ruta_carpeta + nav_home);
				armMoveGroup.execute(msg);*/

			
					
				// Vuelo a posicion cercana de grasping	
				do{
					armMoveGroup.setPoseTarget(pre_grasping_pose);
					std::cout << "Planificando preagarre..." << std::endl;
					armMoveGroup.plan(armPlan);
					std::cout << "Inserta 'n' para replanificar" << std::endl;
					std::cout << "Inserta 's' para ejecutar la planificacion" << std::endl;
					std::cout << "Inserta otra letra para salir" << std::endl;
					std::cin >> inputChar;
				} while(inputChar == 'n');

				if(inputChar == 's'){
					std::cout << "EJECUTANDO PRE-AGARRE" << std::endl;
					armMoveGroup.execute(armPlan);
					
					// Grasping desde la posicion cercana
					do{
						armMoveGroup.setPoseTarget(grasping_pose);
						std::cout << "Planificando agarre..." << std::endl;
						armMoveGroup.plan(armPlan);
						std::cout << "Inserta 'n' para replanificar" << std::endl;
						std::cout << "Inserta 's' para ejecutar la planificacion" << std::endl;
						std::cout << "Inserta otra letra para salir" << std::endl;
						std::cin >> inputChar;
					} while(inputChar == 'n');

					if(inputChar == 's'){
						std::cout << "EJECUTANDO AGARRE" << std::endl;
						armMoveGroup.execute(armPlan);
						
						// Grasping objecto en real
						std::cout << "CERRANDO PINZA" << std::endl;
						std_msgs::Float64 grasping_value;
						//grasping_value.data = jointGripper;
						//pub_jointValue.publish(grasping_value);

						// Con objeto a posicion anterior
						do{
							armMoveGroup.setPoseTarget(pre_grasping_pose);
							std::cout << "Planificando post-agarre..." << std::endl;
							armMoveGroup.plan(armPlan);
							std::cout << "Inserta 'n' para replanificar" << std::endl;
							std::cout << "Inserta 's' para ejecutar la planificacion" << std::endl;
							std::cout << "Inserta otra letra para salir" << std::endl;
							std::cin >> inputChar;
						} while(inputChar == 'n');

						if(inputChar == 's'){
							std::cout << "EJECUTANDO POST-AGARRE" << std::endl;
							armMoveGroup.execute(armPlan);

							// Vuelta a pic
							do{
								armMoveGroup.setPoseTarget(pic_point_returnPoint_pose);
								std::cout << "Planificando vuelta a picture..." << std::endl;
								armMoveGroup.plan(armPlan);
								std::cout << "Inserta 'n' para replanificar" << std::endl;
								std::cout << "Inserta 's' para ejecutar la planificacion" << std::endl;
								std::cout << "Inserta otra letra para salir" << std::endl;
								std::cin >> inputChar;
							} while(inputChar == 'n');

							if(inputChar == 's'){

/*					std::cout << "EJECUTANDO PRE-AGARRE" << std::endl;
					armMoveGroup.setPoseTarget(pre_grasping_pose);
					armMoveGroup.plan(armPlan);
					armMoveGroup.execute(armPlan);

					std::cout << "EJECUTANDO AGARRE" << std::endl;
					armMoveGroup.setPoseTarget(grasping_pose);
					armMoveGroup.plan(armPlan);
					armMoveGroup.execute(armPlan);

					// Grasping objecto en real
					std::cout << "CERRANDO PINZA" << std::endl;
					std_msgs::Float64 grasping_value;
					grasping_value.data = jointGripper;
					pub_jointValue.publish(grasping_value);

					std::cout << "EJECUTANDO POST-AGARRE" << std::endl;
					armMoveGroup.setPoseTarget(pre_grasping_pose);
					armMoveGroup.plan(armPlan);
					armMoveGroup.execute(armPlan);*/




								std::cout << "EJECUTANDO VUELTA A PICTURE" << std::endl;
								armMoveGroup.execute(armPlan);

								std::cout << "EJECUTANDO VUELTA A HOME" << std::endl;
								msg = executeSavedTrajectory (ruta_carpeta + pic_home);
								armMoveGroup.execute(msg);
								
								std::cout << "EJECUTANDO PRE-PLACE" << std::endl;
								msg = executeSavedTrajectory (ruta_carpeta + fase1);
								armMoveGroup.execute(msg);

									
								//std::cout << "EJECUTANDO PLACE" << std::endl;
								//msg = executeSavedTrajectory (ruta_carpeta + fase2);
								//armMoveGroup.execute(msg);


								// Grasping objecto en real y simulacion
								//std::cout << "ABRIENDO PINZA" << std::endl;
								//grasping_value.data = 0.0;
								//pub_jointValue.publish(grasping_value);						
								//ros::Duration(1.0).sleep();					

										
								std::cout << "EJECUTANDO POST-PLACE" << std::endl;
								//msg = executeSavedTrajectory (ruta_carpeta + fase3);
								//armMoveGroup.execute(msg);


								std::cout << "EJECUTANDO REGRESO A INICIO" << std::endl;
								msg = executeSavedTrajectory(ruta_carpeta+fase4);
								armMoveGroup.execute(msg);


								/*std::cout << "EJECUTANDO REGRESO A NAVEGACION" << std::endl;
								msg = executeSavedTrajectory (ruta_carpeta + home_nav);
								armMoveGroup.execute(msg);*/	
							}

								
						}
					}
				}
				
				
				

				
				//gripperMoveGroup.execute(gripperPlan);


				//===========================================================================
				// PLANNING MOVEMENT - OBJECT PLACEMENT
		/*
				geometry_msgs::PoseStamped lectura = armMoveGroup.getCurrentPose("ee_link");
				std::cout << "Pose leida: " << lectura << std::endl;

				// planning movement to place object grasped
				geometry_msgs::Pose place_pose1;
				place_pose1.position.x = -0.0185307;
				place_pose1.position.y = -0.196758;
				place_pose1.position.z = 0.603661;
				place_pose1.orientation.x = 0.535391;
				place_pose1.orientation.y = 0.499153;
				place_pose1.orientation.z = 0.007156;
				place_pose1.orientation.w = 0.546285;
		*/

				
				

				/*joint_group_positions[0] = 0;
				gripperMoveGroup.setJointValueTarget(joint_group_positions);
				gripperMoveGroup.plan(gripperPlan);
				gripperMoveGroup.execute(gripperPlan);*/

				/*
				geometry_msgs::Pose place_pose2;
				place_pose2.position.x = 0.134471;
				place_pose2.position.y = -0.491793;
				place_pose2.position.z = 0.488863;
				place_pose2.orientation.x = -0.499402;
				place_pose2.orientation.y = 0.499402;
				place_pose2.orientation.z = 0.500198;
				place_pose2.orientation.w = 0.500995;

				do{
					armMoveGroup.setPoseTarget(place_pose2);
					std::cout << "Planificando agarre..." << std::endl;
					armMoveGroup.plan(armPlan);
					std::cout << "Inserta 'r' para replanificar" << std::endl;
					std::cout << "Inserta 'e' para ejecutar la planificacion" << std::endl;
					std::cout << "Inserta otra letra para salir" << std::endl;
					std::cin >> inputChar;
				} while(inputChar == 'r');

				if(inputChar == 'e'){
					std::cout << "Ejecutando movimiento de place..." << std::endl;
					armMoveGroup.execute(armPlan);
				}

				// GRASPING EXECUTION - PLACE
				grasping_value.data = 0.0;
				pub_jointValue.publish(grasping_value);//publish joint value to gripper controller

				

				//===========================================================================
				// PLANNING MOVEMENT - RETURN ROBOT TO INITIAL POSE

				// planning movement to place object grasped
				geometry_msgs::Pose return_pose;
				return_pose.orientation.w = 1.0;
				return_pose.position.x = 0.28;
				return_pose.position.y = -0.2;
				return_pose.position.z = 0.5;

				do{
					armMoveGroup.setPoseTarget(return_pose);
					std::cout << "Planificando agarre..." << std::endl;
					armMoveGroup.plan(armPlan);
					std::cout << "Inserta 'r' para replanificar" << std::endl;
					std::cout << "Inserta 'e' para ejecutar la planificacion" << std::endl;
					std::cout << "Inserta otra letra para salir" << std::endl;
					std::cin >> inputChar;
				} while(inputChar == 'r');

				if(inputChar == 'e'){
					std::cout << "Ejecutando movimiento de place..." << std::endl;
					armMoveGroup.execute(armPlan);
				}*/
				
				std::cout << "MORE OBJECTS ? " << std::endl;

			}
			//==========================================================================

			// publish state: end of grasping movement
			std_msgs::Int64 state_end_of_robot_grasping;
		    	state_end_of_robot_grasping.data = 1;
		    	pub_end_of_robot_grasping.publish(state_end_of_robot_grasping);
		}
	}
	return;
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "planning_robot_movement");
	ros::NodeHandle n;	
	ros::AsyncSpinner spinner(1);
  	spinner.start();

	pub_jointValue = n.advertise<std_msgs::Float64>("/valoresArticularesGripper", 100);
	pub_end_of_robot_grasping = n.advertise<std_msgs::Int64>("/end_of_robot_grasping_msg", 100);
	pub_grasping = n.advertise<robotiq_2f_gripper_control::Robotiq2FGripper_robot_output>("/Robotiq2FGripperRobotOutput", 100);
	pub_last_point_robot = n.advertise<geometry_msgs::PoseStamped>("/ur5e/last_point_robot", 100);

  	for(;;){
  		planning_robot();//inicio la planificacion de movimientos 	
  	}

	ros::spin();

	return 0;
}



/******************************************************************************************************************************************/
//PARA MEMORIZAR LAS TRAYECTORIAS

//Guardar
/*moveit_msgs::RobotTrajectory msg = armPlan.trajectory_;
std::cout<<msg<<std::endl;
std::vector<trajectory_msgs::JointTrajectoryPoint> trajectory_points = msg.joint_trajectory.points;                            
std::cout<<"The size of the vectorSize is"<<trajectory_points.size()<<std::endl;

// Write to File
rosbag::Bag bag;
bag.open(ruta_carpeta+nav_home, rosbag::bagmode::Write);			    
bag.write("trajectory_msgs/JointTrajectory", ros::Time::now(), msg);
bag.close();*/

//Ejecutar
/*
rosbag::Bag bag1;
bag1.open(ruta_carpeta+nav_home, rosbag::bagmode::Read);
moveit_msgs::RobotTrajectory msg1;
for(rosbag::MessageInstance const m : rosbag::View(bag1))
{
	std::string imgTopic = m.getTopic();
	std::cout<<"Topic: "<<imgTopic<<std::endl;
	if ("trajectory_msgs/JointTrajectory" == imgTopic){
		msg1 = *m.instantiate<moveit_msgs::RobotTrajectory>();
	}
}
bag1.close();

armMoveGroup.execute(msg1);
*/



/*

Pose de agarrar objeto
-0.0460913
  0.490865
-0.0512489

 -0.316193  -0.936196   0.153493
 -0.869835   0.350673   0.347009
 -0.378694 -0.0237915  -0.925216





Pose leida tras.: header: 
  seq: 0
  stamp: 1634891210.757615915
  frame_id: world
pose: 
  position: 
    x: -0.0769483
    y: 0.421494
    z: 0.133958
  orientation: 
    x: 0.254991
    y: 0.65037
    z: 0.0114341
    w: 0.715449

 
 
1.50759 1.18056 1.15464 */

