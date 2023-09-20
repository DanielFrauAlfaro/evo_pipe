#!/bin/sh

if [ -n "$DESTDIR" ] ; then
    case $DESTDIR in
        /*) # ok
            ;;
        *)
            /bin/echo "DESTDIR argument must be absolute... "
            /bin/echo "otherwise python's distutils will bork things."
            exit 1
    esac
fi

echo_and_run() { echo "+ $@" ; "$@" ; }

echo_and_run cd "/daniel/Desktop/evo_pipe/catkin_ws/src/robotiq/robotiq_3f_gripper_control"

# ensure that Python install destination exists
echo_and_run mkdir -p "$DESTDIR/daniel/Desktop/evo_pipe/catkin_ws/install/lib/python3/dist-packages"

# Note that PYTHONPATH is pulled from the environment to support installing
# into one location when some dependencies were installed in another
# location, #123.
echo_and_run /usr/bin/env \
    PYTHONPATH="/daniel/Desktop/evo_pipe/catkin_ws/install/lib/python3/dist-packages:/daniel/Desktop/evo_pipe/catkin_ws/build/lib/python3/dist-packages:$PYTHONPATH" \
    CATKIN_BINARY_DIR="/daniel/Desktop/evo_pipe/catkin_ws/build" \
    "/usr/bin/python3" \
    "/daniel/Desktop/evo_pipe/catkin_ws/src/robotiq/robotiq_3f_gripper_control/setup.py" \
    egg_info --egg-base /daniel/Desktop/evo_pipe/catkin_ws/build/robotiq/robotiq_3f_gripper_control \
    build --build-base "/daniel/Desktop/evo_pipe/catkin_ws/build/robotiq/robotiq_3f_gripper_control" \
    install \
    --root="${DESTDIR-/}" \
    --install-layout=deb --prefix="/daniel/Desktop/evo_pipe/catkin_ws/install" --install-scripts="/daniel/Desktop/evo_pipe/catkin_ws/install/bin"
