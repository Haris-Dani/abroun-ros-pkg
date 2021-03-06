/*********************************************************************
*
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
* Author: Sachin Chitta
*********************************************************************/

#ifndef GAFFA_ARM_IK_CONSTRAINT_AWARE_H
#define GAFFA_ARM_IK_CONSTRAINT_AWARE_H

#include <angles/angles.h>
#include <gaffa_arm_kinematics/gaffa_arm_kinematics.h>

//Pose command for the ik node
#include <kinematics_msgs/GetConstraintAwarePositionIK.h>
#include <kinematics_msgs/GetKinematicSolverInfo.h>
#include <kinematics_msgs/GetPositionFK.h>

#include <boost/shared_ptr.hpp>

#include <planning_environment/monitors/planning_monitor.h>
#include <planning_models/kinematic_model.h>

#include <motion_planning_msgs/DisplayTrajectory.h>
#include <motion_planning_msgs/LinkPadding.h>
#include <motion_planning_msgs/ArmNavigationErrorCodes.h>

namespace gaffa_arm_kinematics
{
class GaffaArmIKConstraintAware : public gaffa_arm_kinematics::GaffaArmKinematics
{
public:

  /** @class
   *  @brief ROS/KDL based interface for the inverse kinematics of the Gaffa arm
   *  @author Sachin Chitta <sachinc@willowgarage.com>
   *
   *  This class provides a ROS/KDL interface to the inverse kinematics of the Gaffa arm. 
   *  It will compute a collision free solution to the inverse kinematics of the Gaffa arm. 
   *  The collision environment needs to be active for this method to work. This requires the presence of a node 
   *  that is publishing collision maps. 
   *  To use this node, you must have a roscore running with a robot description available from the ROS param server. 
   */
  GaffaArmIKConstraintAware();

  virtual ~GaffaArmIKConstraintAware()
	{
    if (planning_monitor_)
      delete planning_monitor_;
    if (collision_models_)
      delete collision_models_;
	};

  /**
   * @brief This method searches for and returns the closest solution to the initial guess in the first set of solutions it finds. 
   *
   * @return < 0 if no solution is found
   * @param q_in The initial guess for the inverse kinematics solution. The solver uses the joint value q_init(gaffa_ik_->free_angle_) as 
   * as an input to the inverse kinematics. gaffa_ik_->free_angle_ can either be 0 or 2 corresponding to the shoulder pan or shoulder roll angle 
   * @param p_in A KDL::Frame representation of the position of the end-effector for which the IK is being solved.
   * @param q_out A std::vector of KDL::JntArray containing all found solutions.  
   * @param timeout The amount of time (in seconds) to spend looking for a solution.
   */  
  int CartToJntSearch(const KDL::JntArray& q_in, 
                      const KDL::Frame& p_in, 
                      KDL::JntArray &q_out, 
                      const double &timeout, 
                      motion_planning_msgs::ArmNavigationErrorCodes& error_code);

  /**
   * @brief This method searches for and returns the closest solution to the initial guess in the first set of solutions it finds. 
   *
   * @return < 0 if no solution is found
   * @param q_in The initial guess for the inverse kinematics solution. The solver uses the joint value q_init(gaffa_ik_->free_angle_) as 
   * as an input to the inverse kinematics. gaffa_ik_->free_angle_ can either be 0 or 2 corresponding to the shoulder pan or shoulder roll angle 
   * @param p_in A KDL::Frame representation of the position of the end-effector for which the IK is being solved.
   * @param q_out A std::vector of KDL::JntArray containing all found solutions.  
   * @param timeout The amount of time (in seconds) to spend looking for a solution.
   */
  bool getConstraintAwarePositionIK(kinematics_msgs::GetConstraintAwarePositionIK::Request &request, 
                                    kinematics_msgs::GetConstraintAwarePositionIK::Response &response);
private:

  ros::ServiceServer ik_collision_service_;
  planning_environment::CollisionModels *collision_models_;
  planning_environment::PlanningMonitor *planning_monitor_;
  std::string group_;
  bool use_collision_map_;
  ros::Publisher vis_marker_publisher_;
  void contactFound(collision_space::EnvironmentModel::Contact &contact);
  std::vector<std::string> default_collision_links_;
  std::vector<std::string> end_effector_collision_links_;
  void initialPoseCheck(const KDL::JntArray &jnt_array, 
                        const KDL::Frame &p_in,
                        motion_planning_msgs::ArmNavigationErrorCodes &error_code);
  void collisionCheck(const KDL::JntArray &jnt_array, 
                      const KDL::Frame &p_in,
                      motion_planning_msgs::ArmNavigationErrorCodes &error_code);
  void printStringVec(const std::string &prefix, const std::vector<std::string> &string_vector);
  ros::Publisher display_trajectory_publisher_;
  bool visualize_solution_;
  kinematics_msgs::PositionIKRequest ik_request_;
  motion_planning_msgs::OrderedCollisionOperations collision_operations_;
  std::vector<motion_planning_msgs::LinkPadding> link_padding_;
  std::vector<motion_planning_msgs::AllowedContactSpecification> allowed_contacts_;
  motion_planning_msgs::Constraints constraints_;
  bool setup_collision_environment_;
  bool setupCollisionEnvironment(void);
  void advertiseIK();

  bool isReady(motion_planning_msgs::ArmNavigationErrorCodes &error_code);


};
}
#endif
