// ============================================================================
// Name        : rst_pose_to_ros_navmsgs_odometry.cpp
// Author      : Daniel Rudolph <drudolph@techfak.uni-bielefeld.de>
// Description : Recieve  rsb::geometry::Pose and publish them via ros::nav_msgs::Odometry.
// ============================================================================

// ROS
#include <ros/ros.h>
#include <nav_msgs/Odometry.h>

// RSB
#include <rsb/Factory.h>
#include <rsb/converter/Repository.h>
#include <rsb/converter/ProtocolBufferConverter.h>

// RST
#include <rst/geometry/Pose.pb.h>

#include <rsb_to_ros_bridge/rsb_to_ros_time_converter.h>

using namespace std;

// RSB Listener Scope
string rsbListenerScope;

// ROS Publish Topic
string rosPublishTopic;

ros::Publisher rosPosePub;

// program name
const string programName = "rst_pose_to_ros_navmsgs_odometry";

bool rostimenow;
string frameId(""), genericFrameIdSuffix(""),childFrameId("");

void processRstGeometryPose(rsb::EventPtr event) {
  if (event->getType() != "rst::geometry::Pose") {
    return;
  }

  boost::shared_ptr<rst::geometry::Pose> value = boost::static_pointer_cast<rst::geometry::Pose>(event->getData());
  rst::geometry::Translation t = value->translation();
  rst::geometry::Rotation r    = value->rotation();


  nav_msgs::Odometry odom;
  odom.pose.pose.orientation.x = (double) r.qx();
  odom.pose.pose.orientation.y = (double) r.qy();
  odom.pose.pose.orientation.z = (double) r.qz();
  odom.pose.pose.orientation.w = (double) r.qw();
  odom.pose.pose.position.x    = (double) t.x();
  odom.pose.pose.position.y    = (double) t.y();
  odom.pose.pose.position.z    = (double) t.z();
  odom.header.stamp            = getRosTimeFromRsbEvent(event,rostimenow);
  odom.header.frame_id         = frameId.empty() ? event->getScope().getComponents()[0] + genericFrameIdSuffix : frameId;
  odom.child_frame_id          = childFrameId;

  rosPosePub.publish(odom);
}

int main(int argc, char * argv[]) {
  ROS_INFO("Start: %s", programName.c_str());

  // Init ROS
  ros::init(argc, argv, programName);
  ros::NodeHandle node("~");

  node.param<string>("rsb_listener_scope", rsbListenerScope, "/pose");
  node.param<string>("ros_publish_topic", rosPublishTopic, "/pose");
  node.param<bool>("rostimenow", rostimenow, false);
  node.param<string>("frame_id", frameId, "");
  node.param<string>("generic_frame_id_suffix", genericFrameIdSuffix, "/odom");
  node.param<string>("child_frame_id", childFrameId, "/base_link");

  ROS_INFO("rsb_listener_scope: %s", rsbListenerScope.c_str());
  ROS_INFO("ros_publish_topic: %s", rosPublishTopic.c_str());
  ROS_INFO("rostimenow: %s", rostimenow?"True":"False");
  ROS_INFO("frameId: %s", frameId.c_str());
  ROS_INFO("genericFrameIdSuffix: %s", genericFrameIdSuffix.c_str());
  ROS_INFO("child_frame_id: %s", childFrameId.c_str());

  rosPosePub = node.advertise<nav_msgs::Odometry>(rosPublishTopic, 1);

  rsb::Factory& factory = rsb::getFactory();

  boost::shared_ptr<rsb::converter::ProtocolBufferConverter<rst::geometry::Pose> >
  converter(new rsb::converter::ProtocolBufferConverter<rst::geometry::Pose>());
  rsb::converter::converterRepository<std::string>()->registerConverter(converter);

  // Prepare RSB listener
  rsb::ListenerPtr poseListener = factory.createListener(rsbListenerScope);
  poseListener->addHandler(rsb::HandlerPtr(new rsb::EventFunctionHandler(&processRstGeometryPose)));

  ros::spin();

  return 0;
}
