execute_process(COMMAND "/daniel/Desktop/evo_pipe/catkin_ws/build/ros_control/rqt_controller_manager/catkin_generated/python_distutils_install.sh" RESULT_VARIABLE res)

if(NOT res EQUAL 0)
  message(FATAL_ERROR "execute_process(/daniel/Desktop/evo_pipe/catkin_ws/build/ros_control/rqt_controller_manager/catkin_generated/python_distutils_install.sh) returned error code ")
endif()
