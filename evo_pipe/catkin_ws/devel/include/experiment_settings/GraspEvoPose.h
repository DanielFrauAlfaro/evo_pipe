// Generated by gencpp from file experiment_settings/GraspEvoPose.msg
// DO NOT EDIT!


#ifndef EXPERIMENT_SETTINGS_MESSAGE_GRASPEVOPOSE_H
#define EXPERIMENT_SETTINGS_MESSAGE_GRASPEVOPOSE_H


#include <string>
#include <vector>
#include <memory>

#include <ros/types.h>
#include <ros/serialization.h>
#include <ros/builtin_message_traits.h>
#include <ros/message_operations.h>

#include <std_msgs/Header.h>
#include <geometry_msgs/Vector3.h>
#include <geometry_msgs/Pose.h>

namespace experiment_settings
{
template <class ContainerAllocator>
struct GraspEvoPose_
{
  typedef GraspEvoPose_<ContainerAllocator> Type;

  GraspEvoPose_()
    : header()
    , graspPosePoints()
    , midPointPose()  {
    }
  GraspEvoPose_(const ContainerAllocator& _alloc)
    : header(_alloc)
    , graspPosePoints(_alloc)
    , midPointPose(_alloc)  {
  (void)_alloc;
    }



   typedef  ::std_msgs::Header_<ContainerAllocator>  _header_type;
  _header_type header;

   typedef std::vector< ::geometry_msgs::Vector3_<ContainerAllocator> , typename std::allocator_traits<ContainerAllocator>::template rebind_alloc< ::geometry_msgs::Vector3_<ContainerAllocator> >> _graspPosePoints_type;
  _graspPosePoints_type graspPosePoints;

   typedef  ::geometry_msgs::Pose_<ContainerAllocator>  _midPointPose_type;
  _midPointPose_type midPointPose;





  typedef boost::shared_ptr< ::experiment_settings::GraspEvoPose_<ContainerAllocator> > Ptr;
  typedef boost::shared_ptr< ::experiment_settings::GraspEvoPose_<ContainerAllocator> const> ConstPtr;

}; // struct GraspEvoPose_

typedef ::experiment_settings::GraspEvoPose_<std::allocator<void> > GraspEvoPose;

typedef boost::shared_ptr< ::experiment_settings::GraspEvoPose > GraspEvoPosePtr;
typedef boost::shared_ptr< ::experiment_settings::GraspEvoPose const> GraspEvoPoseConstPtr;

// constants requiring out of line definition



template<typename ContainerAllocator>
std::ostream& operator<<(std::ostream& s, const ::experiment_settings::GraspEvoPose_<ContainerAllocator> & v)
{
ros::message_operations::Printer< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >::stream(s, "", v);
return s;
}


template<typename ContainerAllocator1, typename ContainerAllocator2>
bool operator==(const ::experiment_settings::GraspEvoPose_<ContainerAllocator1> & lhs, const ::experiment_settings::GraspEvoPose_<ContainerAllocator2> & rhs)
{
  return lhs.header == rhs.header &&
    lhs.graspPosePoints == rhs.graspPosePoints &&
    lhs.midPointPose == rhs.midPointPose;
}

template<typename ContainerAllocator1, typename ContainerAllocator2>
bool operator!=(const ::experiment_settings::GraspEvoPose_<ContainerAllocator1> & lhs, const ::experiment_settings::GraspEvoPose_<ContainerAllocator2> & rhs)
{
  return !(lhs == rhs);
}


} // namespace experiment_settings

namespace ros
{
namespace message_traits
{





template <class ContainerAllocator>
struct IsMessage< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
  : TrueType
  { };

template <class ContainerAllocator>
struct IsMessage< ::experiment_settings::GraspEvoPose_<ContainerAllocator> const>
  : TrueType
  { };

template <class ContainerAllocator>
struct IsFixedSize< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
  : FalseType
  { };

template <class ContainerAllocator>
struct IsFixedSize< ::experiment_settings::GraspEvoPose_<ContainerAllocator> const>
  : FalseType
  { };

template <class ContainerAllocator>
struct HasHeader< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
  : TrueType
  { };

template <class ContainerAllocator>
struct HasHeader< ::experiment_settings::GraspEvoPose_<ContainerAllocator> const>
  : TrueType
  { };


template<class ContainerAllocator>
struct MD5Sum< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
{
  static const char* value()
  {
    return "edfddb404e31fac0fe15cbe5e0286026";
  }

  static const char* value(const ::experiment_settings::GraspEvoPose_<ContainerAllocator>&) { return value(); }
  static const uint64_t static_value1 = 0xedfddb404e31fac0ULL;
  static const uint64_t static_value2 = 0xfe15cbe5e0286026ULL;
};

template<class ContainerAllocator>
struct DataType< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
{
  static const char* value()
  {
    return "experiment_settings/GraspEvoPose";
  }

  static const char* value(const ::experiment_settings::GraspEvoPose_<ContainerAllocator>&) { return value(); }
};

template<class ContainerAllocator>
struct Definition< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
{
  static const char* value()
  {
    return "Header header\n"
"geometry_msgs/Vector3[] graspPosePoints\n"
"geometry_msgs/Pose midPointPose\n"
"\n"
"================================================================================\n"
"MSG: std_msgs/Header\n"
"# Standard metadata for higher-level stamped data types.\n"
"# This is generally used to communicate timestamped data \n"
"# in a particular coordinate frame.\n"
"# \n"
"# sequence ID: consecutively increasing ID \n"
"uint32 seq\n"
"#Two-integer timestamp that is expressed as:\n"
"# * stamp.sec: seconds (stamp_secs) since epoch (in Python the variable is called 'secs')\n"
"# * stamp.nsec: nanoseconds since stamp_secs (in Python the variable is called 'nsecs')\n"
"# time-handling sugar is provided by the client library\n"
"time stamp\n"
"#Frame this data is associated with\n"
"string frame_id\n"
"\n"
"================================================================================\n"
"MSG: geometry_msgs/Vector3\n"
"# This represents a vector in free space. \n"
"# It is only meant to represent a direction. Therefore, it does not\n"
"# make sense to apply a translation to it (e.g., when applying a \n"
"# generic rigid transformation to a Vector3, tf2 will only apply the\n"
"# rotation). If you want your data to be translatable too, use the\n"
"# geometry_msgs/Point message instead.\n"
"\n"
"float64 x\n"
"float64 y\n"
"float64 z\n"
"================================================================================\n"
"MSG: geometry_msgs/Pose\n"
"# A representation of pose in free space, composed of position and orientation. \n"
"Point position\n"
"Quaternion orientation\n"
"\n"
"================================================================================\n"
"MSG: geometry_msgs/Point\n"
"# This contains the position of a point in free space\n"
"float64 x\n"
"float64 y\n"
"float64 z\n"
"\n"
"================================================================================\n"
"MSG: geometry_msgs/Quaternion\n"
"# This represents an orientation in free space in quaternion form.\n"
"\n"
"float64 x\n"
"float64 y\n"
"float64 z\n"
"float64 w\n"
;
  }

  static const char* value(const ::experiment_settings::GraspEvoPose_<ContainerAllocator>&) { return value(); }
};

} // namespace message_traits
} // namespace ros

namespace ros
{
namespace serialization
{

  template<class ContainerAllocator> struct Serializer< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
  {
    template<typename Stream, typename T> inline static void allInOne(Stream& stream, T m)
    {
      stream.next(m.header);
      stream.next(m.graspPosePoints);
      stream.next(m.midPointPose);
    }

    ROS_DECLARE_ALLINONE_SERIALIZER
  }; // struct GraspEvoPose_

} // namespace serialization
} // namespace ros

namespace ros
{
namespace message_operations
{

template<class ContainerAllocator>
struct Printer< ::experiment_settings::GraspEvoPose_<ContainerAllocator> >
{
  template<typename Stream> static void stream(Stream& s, const std::string& indent, const ::experiment_settings::GraspEvoPose_<ContainerAllocator>& v)
  {
    s << indent << "header: ";
    s << std::endl;
    Printer< ::std_msgs::Header_<ContainerAllocator> >::stream(s, indent + "  ", v.header);
    s << indent << "graspPosePoints[]" << std::endl;
    for (size_t i = 0; i < v.graspPosePoints.size(); ++i)
    {
      s << indent << "  graspPosePoints[" << i << "]: ";
      s << std::endl;
      s << indent;
      Printer< ::geometry_msgs::Vector3_<ContainerAllocator> >::stream(s, indent + "    ", v.graspPosePoints[i]);
    }
    s << indent << "midPointPose: ";
    s << std::endl;
    Printer< ::geometry_msgs::Pose_<ContainerAllocator> >::stream(s, indent + "  ", v.midPointPose);
  }
};

} // namespace message_operations
} // namespace ros

#endif // EXPERIMENT_SETTINGS_MESSAGE_GRASPEVOPOSE_H
