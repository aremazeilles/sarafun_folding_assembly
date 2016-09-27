#include <ros/ros.h>
#include <folding_assembly_controller/controller.hpp>
#include <actionlib/server/simple_action_server.h>
#include <eigen_conversions/eigen_msg.h>
#include <folding_assembly_controller/FoldingAssemblyAction.h>

class FoldingAction
{
protected:
  ros::NodeHandle nh_;
  actionlib::SimpleActionServer<folding_assembly_controller::FoldingAssemblyAction> action_server_;
  std::string action_name_;

  folding_assembly_controller::FoldingAssemblyFeedback feedback_;
  folding_assembly_controller::FoldingAssemblyResult result_;

  foldingController controller_;

  double control_frequency_, desired_contact_force_;

  /* Load the node parameters */
  bool loadParams()
  {
    if(!nh_.getParam("/folding_node/frequency", control_frequency_))
    {
      ROS_ERROR("%s could not retrive control frequency (/folding_node/frequency)!", action_name_.c_str());
      return false;
    }

    return true;
  }

public:

  FoldingAction(std::string name) :
    action_server_(nh_, name, boost::bind(&FoldingAction::executeCB, this, _1), false),
    action_name_(name)
  {
    if(!loadParams())
    {
      ros::shutdown();
      exit(0);
    }

    ROS_INFO("%s is starting the action server...", action_name_.c_str());
    action_server_.start();
    ROS_INFO("%s started!", action_name_.c_str());
  }

  void executeCB(const folding_assembly_controller::FoldingAssemblyGoalConstPtr &goal)
  {
    ros::Rate rate(control_frequency_);
    bool done = false, success = true;
    ros::Time begin_time = ros::Time::now();
    ros::Time begin_loop_time = ros::Time::now();
    ros::Time current_time = ros::Time::now();
    double elapsed_time_sec, total_time_sec;
    Eigen::Vector3d v_out, w_out;
    Eigen::MatrixXd twist_eig(6, 1);
    double vd, wd, theta;
    Eigen::Vector3d contact_point;

    geometry_msgs::Twist output_twist;
    geometry_msgs::Vector3 output_contact_point;

    // Get desired linear and angular velocity
    vd = goal->desired_velocities.linear.x;
    wd = goal->desired_velocities.angular.z;
    desired_contact_force_ = goal->contact_force;

    ROS_INFO("%s received a goal!", action_name_.c_str());

    while(!done)
    {
      if(action_server_.isPreemptRequested() || !ros::ok())
      {
        ROS_WARN("%s was preempted!", action_name_.c_str());
        result_.elapsed_time = (ros::Time::now() - begin_time).toSec();
        action_server_.setPreempted(result_);
        success = false;
        break;
      }

      current_time = ros::Time::now();
      elapsed_time_sec = (current_time - begin_loop_time).toSec();
      controller_.control(vd, wd, desired_contact_force_, v_out, w_out, elapsed_time_sec);
      begin_loop_time = ros::Time::now();

      twist_eig << v_out, w_out;
      controller_.getEstimates(contact_point, theta);
      tf::twistEigenToMsg(twist_eig, output_twist);
      tf::vectorEigenToMsg(contact_point, output_contact_point);

      feedback_.commanded_velocities = output_twist;
      feedback_.contact_point_estimate = output_contact_point;
      feedback_.angle_estimate = theta;
      action_server_.publishFeedback(feedback_);

      // Output vOut and wOut to Yumi
      rate.sleep();
    }

    // Need to define what is a successful folding execution
    if(success)
    {
      result_.elapsed_time = (ros::Time::now() - begin_time).toSec();
      ROS_INFO("%s has succeeded!", action_name_.c_str());
      action_server_.setSucceeded(result_);
    }
  }
};

int main(int argc, char ** argv)
{
  ros::init(argc, argv, "folding_assembly_node");

  FoldingAction folding_action(ros::this_node::getName());
  ROS_INFO("Folding assembly node started with name %s", ros::this_node::getName().c_str());
  ros::spin();
  return 0;
}