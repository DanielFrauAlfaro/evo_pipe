FROM osrf/ros:noetic-desktop-focal

# install ros packages
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-noetic-desktop-full=1.5.0-1* \
    && rm -rf /var/lib/apt/lists/*

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y apt-utils curl wget git bash-completion build-essential sudo && rm -rf /var/lib/apt/lists/*

# Now create the same user as the host itself
ARG UID=1000
ARG GID=1000
RUN addgroup --gid ${GID} daniel
RUN adduser --gecos "ROS User" --disabled-password --uid ${UID} --gid ${GID} daniel
RUN usermod -a -G dialout daniel
RUN mkdir config && echo "ros ALL=(ALL) NOPASSWD: ALL" > config/99_aptget
RUN cp config/99_aptget /etc/sudoers.d/99_aptget
RUN chmod 0440 /etc/sudoers.d/99_aptget && chown root:root /etc/sudoers.d/99_aptget

# Change HOME environment variable
ENV HOME /home/daniel
RUN mkdir -p ${HOME}/ros_ws/src

# Initialize the workspace
RUN cd ${HOME}/ros_ws/src && /bin/bash -c "source /opt/ros/${ROS_DISTRO}/setup.bash; catkin_init_workspace"
RUN cd ${HOME}/ros_ws /bin/bash -c "source source /opt/ros/${ROS_DISTRO}/setup.bash; catkin_make"

# set up environment
COPY ./update_bashrc /sbin/update_bashrc
RUN sudo chmod +x /sbin/update_bashrc ; sudo chown ros /sbin/update_bashrc ; sync ; /bin/bash -c /sbin/update_bashrc ; sudo rm /sbin/update_bashrc

# Install pip
RUN apt-get install -y curl
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
RUN python3.8 get-pip.py

# Install rospkg & empy
RUN pip3 install --target=/opt/ros/noetic/lib/python3/dist-packages rospkg
RUN pip3 install empy

# Compile cv_bridge
RUN cd ${HOME}/ros_ws/src/ & git clone -b melodic https://github.com/ros-perception/vision_opencv.git
RUN cd ${HOME}/ros_ws/ /bin/bash -c "source source /opt/ros/${ROS_DISTRO}/setup.bash; catkin_make -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=/usr/bin/python3 -DPYTHON_INCLUDE_DIR=/usr/include/python3.8m -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.8m.so"

# Put python3 as python
RUN apt update
RUN apt install python-is-python3

# Install pytorch
#RUN pip install torch==1.10.0+cu111 torchvision==0.11.0+cu111 torchaudio==0.10.0 -f https://download.pytorch.org/whl/torch_stable.html

# Install graphic interface
RUN apt-get update
RUN apt-get install -y libgl1-mesa-glx

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get -y install \
    libcanberra-gtk-module \
    libcanberra-gtk3-module
RUN apt-get clean

# Install tkinter
#RUN apt install -y python3-tk

# Install open3d
RUN pip install --ignore-installed PyYAML
RUN pip3 install open3d

# Install realsense camera
RUN cd home/daniel/ros_ws/src/ && git clone -b ros1-legacy https://github.com/IntelRealSense/realsense-ros.git && git clone https://github.com/issaiass/realsense_gazebo_plugin

RUN cd ${HOME}/ros_ws/ /bin/bash -c "source source /opt/ros/${ROS_DISTRO}/setup.bash; catkin_make -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=/usr/bin/python3 -DPYTHON_INCLUDE_DIR=/usr/include/python3.8m -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.8m.so"

RUN apt-get update
RUN apt-get install -y ros-noetic-realsense2-camera && apt-get install ros-noetic-realsense2-description
RUN apt-get install -y usbutils
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE && apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE
RUN apt update && apt-get install -y software-properties-common

RUN curl -L https://github.com/IntelRealSense/librealsense/archive/refs/tags/v2.50.0.tar.gz -o librealsense.tar.gz && tar -xf librealsense.tar.gz && rm -r librealsense.tar.gz
RUN cd librealsense-2.50.0 && mkdir build && cd build && cmake ../ -DFORCE_RSUSB_BACKEND=true -DCMAKE_BUILD_TYPE=release -DBUILD_EXAMPLES=true -DBUILD_GRAPHICAL_EXAMPLES=true && make install
RUN apt-get install -y udev
COPY 99-realsense-libusb.rules /etc/udev/rules.d/

# Install ur arm (generic)
RUN cd home/daniel/ros_ws/src/ && git clone https://github.com/UniversalRobots/Universal_Robots_ROS_Driver.git 
RUN cd home/daniel/ros_ws/src/ && git clone -b melodic-devel https://github.com/ros-industrial/universal_robot.git
RUN apt update -qq
RUN rosdep update
RUN cd home/daniel/ros_ws/ && rosdep install --from-paths src --ignore-src -y
RUN rosdep fix-permissions
RUN cd ${HOME}/ros_ws/ /bin/bash -c "source source /opt/ros/${ROS_DISTRO}/setup.bash; catkin_make -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=/usr/bin/python3 -DPYTHON_INCLUDE_DIR=/usr/include/python3.8m -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.8m.so"

# Install vncviewer
COPY VNC-Viewer-6.22.826-Linux-x64.deb ./
RUN apt install ./VNC-Viewer-6.22.826-Linux-x64.deb

# Install robotiq gripper
RUN cd home/daniel/ros_ws/src/ && git clone https://github.com/TAMS-Group/robotiq.git && git clone https://github.com/orocos/soem.git && git clone https://github.com/juancamilog/canbus_interface.git
RUN rosdep update
RUN rosdep install -y --from-paths ~/ros_ws/src/ --ignore-src --rosdistro noetic

# Install GeoGrasp Evo
#RUN cd ${HOME}/ros_ws/src/ && git clone https://github.com/AUROVA-LAB/aurova_grasping.git

# Give permissions
RUN chown -R 1000:1000 ./../home/daniel/ros_ws/

RUN export CUDA_VISIBLE_DEVICES=[0]


# ${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}graphics

# Install ros_control
RUN cd home/daniel/ros_ws/src/ && git clone https://github.com/ros-controls/ros_control.git

# Install moveit
RUN cd home/daniel/ros_ws/src/ && git clone https://github.com/ros-planning/moveit.git
RUN apt-get install -y ros-noetic-moveit-resources-panda-moveit-config ros-noetic-rosparam-shortcuts

RUN apt-get install -y ros-noetic-ros-control ros-noetic-ros-controllers ros-noetic-controller-manager  ros-noetic-position-controllers  ros-noetic-joint-state-controller  ros-noetic-joint-trajectory-controller  ros-noetic-joint-limits-interface  ros-noetic-transmission-interface
RUN apt-get install -y ros-noetic-rviz-visual-tools

# MODIFICAR LA ULTIMA LINEA CON EL PATH EN EL QUE ESTAN LAS MALLAS
#export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:/daniel/Desktop/iros_2023/
#echo $GAZEBO_MODEL_PATH
RUN echo "export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:/daniel/Desktop/evo_pipe/" >> ~/.bashrc

RUN apt-get install -y psmisc
RUN apt-get update && apt-get update --fix-missing && apt-get install -y gedit nano
RUN apt-get install -y ros-noetic-gazebo-ros-pkgs

# Install opencv
RUN apt update
RUN apt-get install -y libopencv-dev python3-opencv

# Install ros-opencv interface
RUN cd home/daniel/ros_ws/src/ && git clone -b noetic https://github.com/ros-perception/vision_opencv.git

# Install pcl tools
RUN apt-get install -y pcl-tools

# Install Allegro hand for ROS noetic
#COPY ./linux-headers-5.19.0-7.1-liquorix-amd64/ /usr/src/linux-headers-5.19.0-7.1-liquorix-amd64/
#RUN apt-get install -y cmake gcc g++ libpopt-dev udev
#RUN mkdir drivers && cd drivers && wget https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.12.0.tar.gz && tar -xvzf peak-linux-driver-8.12.0.tar.gz
#RUN whoami
#USER root
#RUN /lib/systemd/systemd-udevd --daemon && cd drivers/peak-linux-driver-8.12.0 && make clean && make NET=NO_NETDEV_SUPPORT && make install
#RUN wget https://www.peak-system.com/quick/BasicLinux 
#RUN tar -xvzf BasicLinux 
#RUN cd PCAN-Basic_Linux-4.7.0.3/libpcanbasic && make && make install
#RUN apt-get install -y ros-noetic-libpcan 
RUN ls
RUN cd home/daniel/ros_ws/src/ && git clone https://github.com/gokhansolak/allegro-hand-ros.git
RUN apt-get install -y libpopt-dev ros-noetic-libpcan
RUN mkdir drivers && cd drivers && wget https://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-8.16.0.tar.gz && tar -xvzf peak-linux-driver-8.16.0.tar.gz
#COPY ./linux-headers-5.15.0-76/ /usr/src/linux-headers-5.15.0-76/
#COPY ./linux-headers-5.15.0-76-generic/ /usr/src/linux-headers-5.15.0-76-generic/
#COPY ./linux-hwe-5.15-headers-5.15.0-76/ /usr/src/linux-hwe-5.15-headers-5.15.0-76/

#RUN wget http://archive.ubuntu.com/ubuntu/pool/main/l/linux-hwe-5.15/linux-modules-5.15.0-76-generic_5.15.0-76.83~20.04.1_amd64.deb
#RUN wget http://archive.ubuntu.com/ubuntu/pool/main/l/linux-hwe-5.15/linux-hwe-5.15-source-5.15.0_5.15.0-72.79~20.04.1_all.deb
RUN apt-get update
RUN apt-get install -y linux-headers-5.15.0-87-generic

# RUN wget http://ports.ubuntu.com/pool/main/l/linux-hwe-5.19/linux-headers-5.19.0-50-generic-64k_5.19.0-50.50_arm64.deb
# RUN dpkg -i linux-headers-5.19.0-50-generic-64k_5.19.0-50.50_arm64.deb
# RUN apt install linux-headers-5.19.0-50-generic-64k_5.19.0-50.50_arm64.deb

USER root
RUN /lib/systemd/systemd-udevd --daemon && cd drivers/peak-linux-driver-8.16.0 && make clean && make NET=NO_NETDEV_SUPPORT && make install

COPY libBHand_64 ./libBHand_64/
RUN cd libBHand_64 && make install && ldconfig


## ---------- ULTIMA MODIFICACION DELPATH DE GAZEBO ---------- ##
RUN echo "export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:/daniel/Desktop/evo_pipe/ycb" >> ~/.bashrc

RUN apt-get install -y python3-rospy
RUN pip3 install roboticstoolbox-python

# Allow rviz and gazebo use NVIDIA card
ENV NVIDIA_VISIBLE_DEVICES ${NVIDIA_VISIBLE_DEVICES:-all}
ENV NVIDIA_DRIVER_CAPABILITIES all 


# compute,utility,graphics

# RUN pip3 install plotvol3

#RUN apt-add-repository ppa:dartsim/ppa 
#RUN apt-get update && apt-get install -y libdart6-all-dev 

# Install libcontact.so
#RUN apt-get install -y ros-noetic-contact-states-observer

#RUN apt-get install -y ros-noetic-moveit-visual-tools
#RUN rosdep install --from-paths ~/ros_ws/src/ --ignore-src -r -y


