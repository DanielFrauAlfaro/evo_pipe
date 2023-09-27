#include "experiment_settings/Grasp.h"
#include "geograspevo/GeoGraspEvo.h"
#include <iostream>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/filter.h>
#include <pcl/filters/passthrough.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl_conversions/pcl_conversions.h>
#include <ros/ros.h>
#include "std_msgs/Int32.h"
#include <tf/tf.h>
#include <xmlrpcpp/XmlRpcValue.h>

ros::Subscriber sub;
bool rgbdImgReceived;
sensor_msgs::PointCloud2::ConstPtr pcImg;

std::vector<std::vector<float>> removeExceptNumbers(int numberFingers, std::string apertures_vector){

  // std::string cad = apertures_vector.toXml();
  // std::vector<char> deleteChar = {'v','a','l','u','e','<','>','{','}','/'};  
    
  // for (int i=0; i<deleteChar.size(); i++){
  //   cad.erase(remove(cad.begin(), cad.end(), deleteChar[i]), cad.end());
  // }
  
  // std::stringstream ss(cad);
  // std::vector<float> numbers;
  // std::vector<float> partialNumbers;
  // std::vector<std::vector<float>> correctedNumbers;
  // char ch;
  // int tmp;
  // while(ss >> tmp){
  //   numbers.push_back(tmp);
  //   ss >> ch;
  // }
  
  // int numberGroups = numberFingers - 1;
  // int cont=  0;
  // for (int i=0; i<numberGroups; i++){
  //   for(int j=0; j<2;j++){
  //       partialNumbers.push_back(numbers[cont]);
  //       cont = cont + 1;
  //   }
  //   correctedNumbers.push_back(partialNumbers);
  //   partialNumbers.clear();
  // } 

  std::vector<std::vector<float>> correctedNumbers; 
  std::vector<float> partial = {0.0, 0.0};
  
  for(int i=0; i<numberFingers-1; i++)
  {
    correctedNumbers.push_back(partial);
  }


  
  bool is_number= false;
  int row = 0;
  int col = 0;
  int count = 0;
  std::string aux = "";

  for(int i = 0; i<apertures_vector.size(); i++)
  {
    if(apertures_vector[i] != '{' &&
       apertures_vector[i] != '}' &&
       apertures_vector[i] != ',' &&
       apertures_vector[i] != ' ')
    {
      aux += apertures_vector[i];
      is_number = true;
    }

    else if(is_number)
    {
      is_number = false;

      correctedNumbers[col][row] = std::stof(aux);
      aux = "";
      col++;

      if(col == numberFingers-1)
      {
        col = 0;
        row++;
      }
    }
  }
  
  return correctedNumbers;
}

void PCCallback(const sensor_msgs::PointCloud2::ConstPtr& msg){

  if (msg->height != 0){
    pcImg = msg;
    sub.shutdown();
    rgbdImgReceived = true; 
  }
}

template<typename T, typename U>
void extractInliersCloud(const T & inputCloud,
    const pcl::PointIndices::Ptr & inputCloudInliers, T outputCloud) {
  U extractor;

  extractor.setInputCloud(inputCloud);
  extractor.setIndices(inputCloudInliers);
  extractor.filter(*outputCloud);
}

void processPC(ros::NodeHandle nh, std::string saveFilesPath, int gripTipSize, int numberFingers, int uniqueMobility, int graspsTrack, std::vector<std::vector<float>> apertures, int actualObject){

  // pcl::visualization::PCLVisualizer::Ptr viewer(new pcl::visualization::PCLVisualizer("Cloud viewer"));      
  // viewer->initCameraParameters();
  // viewer->addCoordinateSystem(0.1);
  // viewer->setBackgroundColor (0, 0, 0);

  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
  pcl::fromROSMsg(*pcImg, *cloud);

  // Remove NaN values and make it dense
  std::vector<int> nanIndices;
  pcl::removeNaNFromPointCloud(*cloud, *cloud, nanIndices);

  // Remove background points
  pcl::PassThrough<pcl::PointXYZ> ptFilter;
  ptFilter.setInputCloud(cloud);
  ptFilter.setFilterFieldName("z");
  ptFilter.setFilterLimits(0.0, 0.9);
  ptFilter.filter(*cloud);

  ptFilter.setInputCloud(cloud);
  ptFilter.setFilterFieldName("y");
  ptFilter.setFilterLimits(-0.55, 0.40);
  ptFilter.filter(*cloud);

  ptFilter.setInputCloud(cloud);
  ptFilter.setFilterFieldName("x");
  ptFilter.setFilterLimits(-0.50, 0.30);
  ptFilter.filter(*cloud);
  
  // viewer->addPointCloud<pcl::PointXYZ> (cloud, "sample cloud");
  
  // while (!viewer->wasStopped ()){
  //   viewer->spinOnce (100);
  // }
  
  // viewer->removeAllPointClouds();
  // viewer->removeAllShapes();
  
  // Create the segmentation object for the planar model and set all the parameters
  pcl::SACSegmentation<pcl::PointXYZ> sacSegmentator;
  pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
  pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloudPlane(new pcl::PointCloud<pcl::PointXYZ>());

  sacSegmentator.setModelType(pcl::SACMODEL_PLANE);
  sacSegmentator.setMethodType(pcl::SAC_RANSAC);
  sacSegmentator.setMaxIterations(50);
  sacSegmentator.setDistanceThreshold(0.02);
  sacSegmentator.setInputCloud(cloud);
  sacSegmentator.segment(*inliers, *coefficients);

  // Remove the planar inliers, extract the rest
  pcl::ExtractIndices<pcl::PointXYZ> indExtractor;
  indExtractor.setInputCloud(cloud);
  indExtractor.setIndices(inliers);
  indExtractor.setNegative(false);

  // Get the points associated with the planar surface
  indExtractor.filter(*cloudPlane);

  // Remove the planar inliers, extract the rest
  indExtractor.setNegative(true);
  indExtractor.filter(*cloud);

  // Creating the KdTree object for the search method of the extraction
  pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
  tree->setInputCloud(cloud);

  std::vector<pcl::PointIndices> clusterIndices;
  pcl::EuclideanClusterExtraction<pcl::PointXYZ> ecExtractor;
  ecExtractor.setClusterTolerance(0.01);
  ecExtractor.setMinClusterSize(750);
  ecExtractor.setSearchMethod(tree);
  ecExtractor.setInputCloud(cloud);
  ecExtractor.extract(clusterIndices);
  
  // pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> rgb(cloud, 255, 0, 0);
  // viewer->addPointCloud<pcl::PointXYZ>(cloud, rgb, "Main cloud");

  // while (!viewer->wasStopped ()){
  //   viewer->spinOnce (100);
  // }
  
  // viewer->removeAllPointClouds();
  // viewer->removeAllShapes();
  
  if (clusterIndices.empty()) {
    ROS_INFO("Cluster indices is empty. Can not keep on with the GeoGraspEvo algorithm.");
  }
  else {
    std::vector<pcl::PointIndices>::const_iterator it = clusterIndices.begin();
    int objectNumber = 0;
    std::vector<float> rankings;
    std::vector<GraspEvoContacts> grasps;
    GraspEvoContacts bestGrasp;
    GraspEvoPose bestPose;   
    float ranking;

    // Every cluster found is considered an object
    // Ideado para trabajar con 1 objeto unicamente. CUIDADO
    for (it = clusterIndices.begin(); it != clusterIndices.end(); ++it) {
      pcl::PointCloud<pcl::PointXYZ>::Ptr objectCloud(new pcl::PointCloud<pcl::PointXYZ>());

      if (objectNumber == 0){ 
        for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit)
          objectCloud->points.push_back(cloud->points[*pit]);

        objectCloud->width = objectCloud->points.size();
        objectCloud->height = 1;
        objectCloud->is_dense = true;

        // Create and initialise GeoGraspEvo
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloudPlaneXYZ(new pcl::PointCloud<pcl::PointXYZ>());
        pcl::PointCloud<pcl::PointXYZ>::Ptr objectCloudXYZ(new pcl::PointCloud<pcl::PointXYZ>());
        pcl::copyPointCloud(*cloudPlane, *cloudPlaneXYZ);
        pcl::copyPointCloud(*objectCloud, *objectCloudXYZ);

        GeoGraspEvo geoGraspPoints;
        geoGraspPoints.setBackgroundCloud(cloudPlaneXYZ);
        geoGraspPoints.setObjectCloud(objectCloudXYZ);
        geoGraspPoints.setGripTipSize(gripTipSize);
        geoGraspPoints.setApertures(apertures);
        geoGraspPoints.setNumberFingers(numberFingers);
        geoGraspPoints.setUniqueMobility(uniqueMobility); //Move fingers independent
        geoGraspPoints.setGrasps(graspsTrack); // Keep track only of the best

        
        // ############################## SAVE PCDs ##########################################
        // Guardar la nube del objeto
        //std::cout<<"\n\nGUARDANDO LA NUBE\n\n";
        //pcl::io::savePCDFileASCII (saveFilesPath+std::to_string(actualObject)+"_object.pcd", *objectCloud);
        //pcl::io::savePCDFileASCII (saveFilesPath+std::to_string(actualObject)+"_plane.pcd", *cloudPlane);
        // ###################################################################################


        std::cout<<"\n\n######## COMPROBACIÓN DE PARÁMETROS ######\n";
        std::cout<<"gripTipSize: "<<gripTipSize<<std::endl;

        std::cout<<"apertures\n";
        for(int p = 0; p<apertures.size(); p++)
        {
          for(int k = 0; k<apertures[p].size(); k++)
          {
            std::cout<<apertures[p][k]<<"   ";
          }       
          std::cout<<"\n";
        }
        
        std::cout<<"numberFingers: "<<numberFingers<<std::endl;
        std::cout<<"uniqueMobility: "<<uniqueMobility<<std::endl;
        std::cout<<"graspsTrack: "<<graspsTrack<<std::endl;
        std::cout<<"\n\n######## COMPROBACIÓN DE PARÁMETROS ######\n";

        
        // Calculate grasping points
        geoGraspPoints.compute();
      
        rankings = geoGraspPoints.getRankings();
        grasps = geoGraspPoints.getGrasps();
      
        std::fstream my_file;
        my_file.open(saveFilesPath+std::to_string(actualObject)+"_info.txt", std::ios::app);
        if (!my_file){
          ROS_INFO("ERROR: file was not created.");
        }
        else{
          my_file<<"ALL GRASPING DATA CALCULATED: \n";
          
          for(int i=0;i<grasps.size();i++){
            my_file<<"\t Point "<<i+1<<"/"<<grasps.size()<<": ";
            for(int j=0;j<grasps[i].graspContactPoints.size();j++){
              my_file<<grasps[i].graspContactPoints[j]<<" ";
            }
            my_file<<"with ranking "<<rankings[i]<<"\n";
          }
        
          my_file.close();
        }  
      
        /*for (int i=0; i<grasps.size(); i++){
          std::cout<<"P"<<i<<": ";
          for (int j=0; j<grasps[i].graspContactPoints.size(); j++){
            std::cout<<grasps[i].graspContactPoints[j]<<" ";
          }
          std::cout<<std::endl;
        }
      
        std::cout<<"RR: ";
        for (int i=0; i<rankings.size(); i++){
          std::cout<<rankings[i]<<" ";
        }
        std::cout<<std::endl;*/
      
        // Extract best pair of points
        bestGrasp = geoGraspPoints.getBestGrasp();
        bestPose = geoGraspPoints.getBestGraspPose();   
        ranking = geoGraspPoints.getBestRanking();     

        // Visualize the result
        // pcl::visualization::PointCloudGeometryHandlerXYZ<pcl::PointXYZ> rgb(objectCloud);
        // pcl::visualization::PointCloudGeometryHandlerXYZ<pcl::PointXYZ> planeRGB(cloudPlane);
  
        std::string objectLabel = "";
        std::ostringstream converter;

        converter << objectNumber;
        objectLabel += converter.str();
        objectLabel += "-";

        // viewer->addPointCloud<pcl::PointXYZ>(objectCloud, rgb, objectLabel + "Object");
        // viewer->addPointCloud<pcl::PointXYZ>(cloudPlane, planeRGB, objectLabel + "Plane");

        // for (int i=0; i<bestGrasp.graspContactPoints.size(); i++){
        //   std::string cad = "Best grasp n." + std::to_string(i);
        //   viewer->addSphere(bestGrasp.graspContactPoints[i], 0.01, 255, 255, 255, objectLabel + cad);
        // }      
      
        // Best of the best cout
      
        my_file.open(saveFilesPath+std::to_string(actualObject)+"_info.txt", std::ios::app);
        if (!my_file){
          ROS_INFO("ERROR: file was not created.");
        }
        else{
          my_file<<"BEST GRASPING DATA FOUND: \n";
        
          my_file<<"\t Point: (";
          for(int i=0;i<bestPose.graspPosePoints.size();i++){
            for(int j=0;j<bestPose.graspPosePoints[i].size();j++){
              my_file<<bestPose.graspPosePoints[i][j];
              if(j!=bestPose.graspPosePoints[i].size()-1){
                my_file<<",";
              }
            }
            if(i==bestPose.graspPosePoints.size()-1){
              my_file<<") ";
            }
            else{
              my_file<<") (";
            }
          }
          my_file<<"with ranking "<<ranking<<"\n";
        
          my_file<<"\t Mid Point: \n";
        
          my_file<<"\t\t Pose: ";
          for(int i=0;i<bestPose.midPointPose.translation().size();i++){
            my_file<<bestPose.midPointPose.translation()[i];
            if(i!=bestPose.midPointPose.translation().size()-1){
              my_file<<", ";
            }
          }
        
          my_file<<"\n";
        
          my_file<<"\t\t Orientation: \n";  
        
          Eigen::Quaternionf q1 (bestPose.midPointPose.rotation());
          tf::Quaternion qtf1(q1.x(),q1.y(),q1.z(),q1.w());
          
          my_file<<"\t\t\t Quaternion ->"<<qtf1.x()<<", "<<qtf1.y()<<", "<<qtf1.z()<<", "<<qtf1.w()<<"\n";   
        
          tf::Matrix3x3 m(qtf1);
          double roll, pitch, yaw;
          m.getRPY(roll, pitch, yaw);
                
          my_file<<"\t\t\t RPY ->"<<roll<<", "<<pitch<<", "<<yaw<<"\n";        
          my_file.close();
        }
      
        /*std::cout<<"Best ranking: "<<ranking<<std::endl;
      
        std::cout<<"Best pose: "<<std::endl;
        std::cout<<"GraspPosePoints"<<std::endl;
        for(int i=0;i<bestPose.graspPosePoints.size();i++){
          std::cout<<bestPose.graspPosePoints[i][0]<<" "<<bestPose.graspPosePoints[i][1]<<" "<<bestPose.graspPosePoints[i][2]<<std::endl;
        }
        std::cout<<"MidPointPose: "<<std::endl;
        Eigen::Quaternionf q1 (bestPose.midPointPose.rotation());
        tf::Quaternion qtf1(q1.x(),q1.y(),q1.z(),q1.w());
        std::cout<<bestPose.midPointPose.translation().x()<<" "<<bestPose.midPointPose.translation().y()<<" "<<bestPose.midPointPose.translation().z()<<" "<<qtf1.x()<<" " <<qtf1.y()<<" "<<qtf1.z()<<" "<<qtf1.w()<<std::endl;*/
      
        std::string message = "Best grasping point: (";
        for(int i=0;i<bestPose.graspPosePoints.size();i++){
          for(int j=0;j<bestPose.graspPosePoints[i].size();j++){
            message+=std::to_string(bestPose.graspPosePoints[i][j]);
            if(j!=bestPose.graspPosePoints[i].size()-1){
              message+=",";
            }
          }
          if(i==bestPose.graspPosePoints.size()-1){
            message+=") ";
          }
          else{
            message+=") (";
          }
        }
        message+="with ranking "+std::to_string(ranking)+".";
      
        ROS_INFO(" ");
        //ROS_INFO(message);
        ROS_INFO("%s\n", message.c_str());
        ROS_INFO(" ");

        pcl::ModelCoefficients axeX;
        axeX.values.resize (6);    // We need 6 values
        axeX.values[0] = bestPose.midPointPose.translation()[0];
        axeX.values[1] = bestPose.midPointPose.translation()[1];
        axeX.values[2] = bestPose.midPointPose.translation()[2];
        axeX.values[3] = bestPose.midPointPose.linear()(0, 0);
        axeX.values[4] = bestPose.midPointPose.linear()(1, 0);
        axeX.values[5] = bestPose.midPointPose.linear()(2, 0);

        // viewer->addLine(axeX, objectLabel + "Pose axeX");

        pcl::ModelCoefficients axeY;
        axeY.values.resize (6);    // We need 6 values
        axeY.values[0] = bestPose.midPointPose.translation()[0];
        axeY.values[1] = bestPose.midPointPose.translation()[1];
        axeY.values[2] = bestPose.midPointPose.translation()[2];
        axeY.values[3] = bestPose.midPointPose.linear()(0, 1);
        axeY.values[4] = bestPose.midPointPose.linear()(1, 1);
        axeY.values[5] = bestPose.midPointPose.linear()(2, 1);

        // viewer->addLine(axeY, objectLabel + "Pose axeY");

        pcl::ModelCoefficients axeZ;
        axeZ.values.resize (6);    // We need 6 values
        axeZ.values[0] = bestPose.midPointPose.translation()[0];
        axeZ.values[1] = bestPose.midPointPose.translation()[1];
        axeZ.values[2] = bestPose.midPointPose.translation()[2];
        axeZ.values[3] = bestPose.midPointPose.linear()(0, 2);
        axeZ.values[4] = bestPose.midPointPose.linear()(1, 2);
        axeZ.values[5] = bestPose.midPointPose.linear()(2, 2);

        // viewer->addLine(axeZ, objectLabel + "Pose axeZ");
      
        objectNumber++;
      }
    }

    // while (!viewer->wasStopped())
    //   viewer->spinOnce(100);
      
    // Save result as pcd. For that load complete cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr PC_color_result(new pcl::PointCloud<pcl::PointXYZRGB>());
    for (int i=0;i<cloud->points.size();i++){
      pcl::PointXYZRGB singlePoint;
      singlePoint.x = cloud->points[i].x;
      singlePoint.y = cloud->points[i].y;
      singlePoint.z = cloud->points[i].z;
      singlePoint.r = 255;
      singlePoint.g = 255;
      singlePoint.b = 255;
      
      PC_color_result->push_back(singlePoint);
    }
    for(int i=0;i<cloudPlane->points.size();i++){
      pcl::PointXYZRGB singlePoint;
      singlePoint.x = cloudPlane->points[i].x;
      singlePoint.y = cloudPlane->points[i].y;
      singlePoint.z = cloudPlane->points[i].z;
      singlePoint.r = 127;
      singlePoint.g = 127;
      singlePoint.b = 127;
      
      PC_color_result->push_back(singlePoint);
    }
    
    PC_color_result->width = PC_color_result->points.size();
    PC_color_result->height = 1;
    PC_color_result->is_dense = true;
    
    // Get grasping points
    std::vector<pcl::PointXYZRGB> graspPoints;
    for(int i=0;i<bestPose.graspPosePoints.size(); i++){
      pcl::PointXYZRGB posePoint;
      posePoint.x = bestPose.graspPosePoints[i][0];
      posePoint.y = bestPose.graspPosePoints[i][1];
      posePoint.z = bestPose.graspPosePoints[i][2];
      
      graspPoints.push_back(posePoint);
    }
    
    // Get neighbours of those points and paint them in red
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZRGB>);
    for(int i=0;i<graspPoints.size(); i++){
      pcl::KdTreeFLANN<pcl::PointXYZRGB> kdtree;
      float radius = 0.007;    
      std::vector<int> pointIdxRadiusSearch;
      std::vector<float> pointRadiusSquaredDistance;
      pcl::PointXYZRGB searchPoint(graspPoints[i]);
      
      kdtree.setInputCloud (PC_color_result); 
      kdtree.radiusSearch (searchPoint, radius, pointIdxRadiusSearch, pointRadiusSquaredDistance);

      for (size_t i = 0; i < pointIdxRadiusSearch.size (); ++i){
        pcl::PointXYZRGB point;
        point.x = PC_color_result->points[pointIdxRadiusSearch[i]].x;
        point.y = PC_color_result->points[pointIdxRadiusSearch[i]].y;
        point.z = PC_color_result->points[pointIdxRadiusSearch[i]].z;
        point.r = 255;
        point.g = 0;
        point.b = 0;
        cloud_cluster->points.push_back(point);
      }
        
      cloud_cluster->width = cloud_cluster->points.size ();
      cloud_cluster->height = 1;
      cloud_cluster->is_dense = true;
    }
    
    // ###################### SAVE PCDs ###########################################
    // pcl::PointCloud<pcl::PointXYZRGB>::Ptr save_PC (new pcl::PointCloud<pcl::PointXYZRGB>);
    // *save_PC = *PC_color_result;
    // *save_PC += *cloud_cluster;
    
    //pcl::io::savePCDFileASCII (saveFilesPath+std::to_string(actualObject)+"_PC_graspPoints.pcd", *save_PC);
    // #############################################################################
    
    // pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> rgb(PC_color_result);
    // pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> points_rgb(cloud_cluster);
    // pcl::visualization::PCLVisualizer::Ptr viewer1(new pcl::visualization::PCLVisualizer("Viewer")); 
    // viewer1->addPointCloud<pcl::PointXYZRGB>(PC_color_result, rgb,"Object");
    // viewer1->addPointCloud<pcl::PointXYZRGB>(cloud_cluster, points_rgb,"Points");
    
    // while (!viewer1->wasStopped())
    //   viewer1->spinOnce(100);
    
    // pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> rgb(save_PC); 
    // pcl::visualization::PCLVisualizer::Ptr viewer1(new pcl::visualization::PCLVisualizer("Viewer1")); 
    // viewer1->addPointCloud<pcl::PointXYZRGB>(save_PC, rgb,"Object");
    
    // while (!viewer1->wasStopped())
    //   viewer1->spinOnce(100);

    // Extra code for sending the grasp to the other programs      
    ros::Publisher graspMessage_pub = nh.advertise<experiment_settings::Grasp>("/aurova/grasp/result", 1);
    ros::Time now = ros::Time::now();
    experiment_settings::Grasp graspMessage;
    graspMessage.header.stamp = now;
    graspMessage.header.frame_id = "camera_depth_frame";
    
    experiment_settings::GraspEvoContacts contact;
    sensor_msgs::PointCloud2 points;
    pcl::PointCloud<pcl::PointXYZ>::Ptr points_pcl(new pcl::PointCloud<pcl::PointXYZ>());
    contact.header.stamp = now;
    contact.header.frame_id = "camera_depth_frame";
    for (int j=0; j<grasps[0].graspContactPoints.size(); j++){
          points_pcl->push_back(grasps[0].graspContactPoints[j]);
    }    
    pcl::toROSMsg(*points_pcl, points);
    contact.graspContactPoints = points;
    graspMessage.bestGrasp = contact;            
    
    graspMessage.ranking = ranking;
    
    experiment_settings::GraspEvoPose pose;
    pose.header.stamp = now;
    pose.header.frame_id = "camera_depth_frame";
    std::vector<geometry_msgs::Vector3> posePoints;
    for(int i=0;i<bestPose.graspPosePoints.size(); i++){
      geometry_msgs::Vector3 posePoint;
      posePoint.x = bestPose.graspPosePoints[i][0];
      posePoint.y = bestPose.graspPosePoints[i][1];
      posePoint.z = bestPose.graspPosePoints[i][2];
      
      posePoints.push_back(posePoint);
    }
    pose.graspPosePoints = posePoints;  
      
    geometry_msgs::Pose midposePoint;
    midposePoint.position.x = bestPose.midPointPose.translation().x();
    midposePoint.position.y = bestPose.midPointPose.translation().y();
    midposePoint.position.z = bestPose.midPointPose.translation().z();
    Eigen::Quaternionf q (bestPose.midPointPose.rotation());
    tf::Quaternion qtf(q.x(),q.y(),q.z(),q.w());
    midposePoint.orientation.x = qtf.x();
    midposePoint.orientation.y = qtf.y();
    midposePoint.orientation.z = qtf.z();
    midposePoint.orientation.w = qtf.w();
    pose.midPointPose = midposePoint;
    graspMessage.bestPose = pose;
    
    graspMessage_pub.publish(graspMessage);
    
    int loop = 0;
    while(loop < 2){      
      ros::spinOnce();
      graspMessage_pub.publish(graspMessage);
      ros::Duration(0.5).sleep(); 
      loop = loop + 1;
    }
  }

  std::cout<<"FIN DEL NODO DE GEOGRASP\n\n";
}

int main(int argc, char *argv[]){

  // This must be called before anything else ROS-related
  ros::init(argc, argv, "geograsp_evo");

  // Create a ROS node handle
  ros::NodeHandle nh("~");
  ros::Rate loop_rate(10);
  
  // Get parameters from launch file
  std::string saveFilesPath;
  int gripTipSize, numberFingers, uniqueMobility, graspsTrack;
  
  nh.getParam("save_files_path", saveFilesPath);  
  nh.getParam("geograspEvo_grip_tip_size", gripTipSize);
  nh.getParam("geograspEvo_set_number_fingers", numberFingers);
  nh.getParam("geograspEvo_set_unique_mobility", uniqueMobility);
  nh.getParam("geograspEvo_set_grasps_track", graspsTrack);

  std::string aperturesVector;
  nh.getParam("geograspEvo_set_apertures", aperturesVector);
  std::vector<std::vector<float>> apertures = removeExceptNumbers(numberFingers, aperturesVector);

  for(int i = 0; i<apertures.size(); i++)
  {
    for(int j=0; j<apertures[i].size(); j++)
    {
      std::cout<<apertures[i][j]<<" hola ";
    }
    std::cout<<std::endl;
  }

  int actualObject = 0;
   
  while(ros::ok()){  
    
    // Wait for image to be processed
    rgbdImgReceived = false;
    sub = nh.subscribe("/aurova/image/RGBD", 1, PCCallback);
    while (rgbdImgReceived == false){
      ros::spinOnce();  
    }    

    // Get grasping points
    processPC(nh, saveFilesPath, gripTipSize, numberFingers, uniqueMobility, graspsTrack, apertures, actualObject);
  
    actualObject = actualObject + 1;
    
  }
      
  return 0;
}
