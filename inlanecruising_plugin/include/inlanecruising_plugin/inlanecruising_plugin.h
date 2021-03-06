#pragma once

/*
 * Copyright (C) 2019-2020 LEIDOS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <vector>
#include <cav_msgs/TrajectoryPlan.h>
#include <cav_msgs/TrajectoryPlanPoint.h>
#include <cav_msgs/Plugin.h>
#include <boost/shared_ptr.hpp>
#include <carma_utils/CARMAUtils.h>
#include <boost/geometry.hpp>
#include <carma_wm/Geometry.h>
#include <cav_srvs/PlanTrajectory.h>
#include <carma_wm/WMListener.h>
#include <functional>
#include <inlanecruising_plugin/smoothing/SplineI.h>

#include "inlanecruising_config.h"
#include "third_party_library/spline.h"

namespace inlanecruising_plugin
{
using PublishPluginDiscoveryCB = std::function<void(const cav_msgs::Plugin&)>;

/**
 * \brief Convenience class for pairing 2d points with speeds
 */ 
struct PointSpeedPair
{
  lanelet::BasicPoint2d point;
  double speed = 0;
};
/**
 * \brief Class representing a curve in space defined by a set of discrete points in the specified frame
 */ 
struct DiscreteCurve
{
  Eigen::Isometry2d frame; // Frame which points are in
  std::vector<PointSpeedPair> points;
};

/**
 * \brief Class containing primary business logic for the In-Lane Cruising Plugin
 * 
 */ 
class InLaneCruisingPlugin
{
public:
  /**
   * \brief Constructor
   * 
   * \param wm Pointer to intialized instance of the carma world model for accessing semantic map data
   * \param config The configuration to be used for this object
   * \param plugin_discovery_publisher Callback which will publish the current plugin discovery state
   */ 
  InLaneCruisingPlugin(carma_wm::WorldModelConstPtr wm, InLaneCruisingPluginConfig config,
                       PublishPluginDiscoveryCB plugin_discovery_publisher);

  /**
   * \brief Service callback for trajectory planning
   * 
   * \param req The service request
   * \param resp The service response
   * 
   * \return True if success. False otherwise
   */ 
  bool plan_trajectory_cb(cav_srvs::PlanTrajectoryRequest& req, cav_srvs::PlanTrajectoryResponse& resp);

  /**
   * \brief Method to call at fixed rate in execution loop. Will publish plugin discovery updates
   * 
   * \return True if the node should continue running. False otherwise
   */ 
  bool onSpin();

  /**
   * \brief Applies the provided speed limits to the provided speeds such that each element is capped at its corresponding speed limit if needed
   * 
   * \param speeds The speeds to limit
   * \param speed_limits The speed limits to apply. Must have the same size as speeds
   * 
   * \return The capped speed limits. Has the same size as speeds
   */ 
  std::vector<double> apply_speed_limits(const std::vector<double> speeds, const std::vector<double> speed_limits);

  /**
   * \brief Returns a 2D coordinate frame which is located at p1 and oriented so p2 lies on the +X axis
   * 
   * \param p1 The origin point for the frame in the parent frame
   * \param p2 A point in the parent frame that will define the +X axis relative to p1
   * 
   * \return A 2D coordinate frame transform
   */ 
  Eigen::Isometry2d compute_heading_frame(const lanelet::BasicPoint2d& p1, const lanelet::BasicPoint2d& p2);

  /**
   * \brief Reduces the input points to only those points that fit within the provided time boundary
   * 
   * \param points The input point speed pairs to reduce
   * \param time_span The time span in seconds which the output points will fit within
   * 
   * \return The subset of points that fit within time_span
   */ 
  std::vector<PointSpeedPair> constrain_to_time_boundary(const std::vector<PointSpeedPair>& points, double time_span);

  /**
   * \brief Method converts a list of lanelet centerline points and current vehicle state into a usable list of trajectory points for trajectory planning
   * 
   * \param points The set of points that define the current lane the vehicle is in and are defined based on the request planning maneuvers. 
   *               These points must be in the same lane as the vehicle and must extend in front of it though it is fine if they also extend behind it. 
   * \param state The current state of the vehicle
   * 
   * \return A list of trajectory points to send to the carma planning stack
   */ 
  std::vector<cav_msgs::TrajectoryPlanPoint>
  compose_trajectory_from_centerline(const std::vector<PointSpeedPair>& points, const cav_msgs::VehicleState& state);

  /**
   * \brief Method combines input points, times, orientations, and an absolute start time to form a valid carma platform trajectory
   * 
   * NOTE: All input vectors must be the same size. The output vector will take this size.
   * 
   * \param points The points in the map frame that the trajectory will follow. Units m
   * \param times The times which at the vehicle should arrive at the specified points. First point should have a value of 0. Units s
   * \param yaws The orientation the vehicle should achieve at each point. Units radians
   * \param startTime The absolute start time which will be used to update the input relative times. Units s
   * 
   * \return A list of trajectory points built from the provided inputs.
   */ 
  std::vector<cav_msgs::TrajectoryPlanPoint> trajectory_from_points_times_orientations(
      const std::vector<lanelet::BasicPoint2d>& points, const std::vector<double>& times,
      const std::vector<double>& yaws, ros::Time startTime);

  /**
   * \brief Converts a set of requested LANE_FOLLOWING maneuvers to point speed limit pairs. 
   * 
   * \param maneuvers The list of maneuvers to convert
   * \param max_starting_downtrack The maximum downtrack that is allowed for the first maneuver. This should be set to the vehicle position or earlier.
   *                               If the first maneuver exceeds this then it's downtrack will be shifted to this value.
   * 
   * \param wm Pointer to intialized world model for semantic map access
   * 
   * \return List of centerline points paired with speed limits
   */ 
  std::vector<PointSpeedPair> maneuvers_to_points(const std::vector<cav_msgs::Maneuver>& maneuvers,
                                                  double max_starting_downtrack,
                                                  const carma_wm::WorldModelConstPtr& wm);

  /**
   * \brief Returns the nearest point to the provided vehicle pose in the provided list
   * 
   * \param points The points to evaluate
   * \param state The current vehicle state
   * 
   * \return index of nearest point in points
   */ 
  int getNearestPointIndex(const std::vector<PointSpeedPair>& points, const cav_msgs::VehicleState& state);

  /**
   * \brief Helper method to split a list of PointSpeedPair into separate point and speed lists 
   */ 
  void splitPointSpeedPairs(const std::vector<PointSpeedPair>& points, std::vector<lanelet::BasicPoint2d>* basic_points,
                            std::vector<double>* speeds);

  /**
   * \brief Computes a spline based on the provided points
   * 
   * \param basic_points The points to use for fitting the spline
   * 
   * \return A spline which has been fit to the provided points
   */ 
  std::unique_ptr<smoothing::SplineI> compute_fit(const std::vector<lanelet::BasicPoint2d>& basic_points);

  /**
   * \brief Calculates a list of DiscreteCurve objects from the input points where a new curve is present everytime the dx of the previous point went negative.
   *        Each curve is defined relative to a frame oriented on the current point and next point. 
   *        This ensures that splines can be fit on each sub-curve. The endpoint and start point of each curve is shared.
   * 
   * \param basic_points The points to split into sub-curves
   * 
   * \return The vector of sub-curves
   */ 
  std::vector<DiscreteCurve> compute_sub_curves(const std::vector<PointSpeedPair>& basic_points);

  /**
   * \brief Computes the transform T_m_p for the point and yaw in a curve which has a transform with the map frame.
   *        This is effectively a method for computing position and orientation of points in a DiscreteCurve frame in the map frame.
   * 
   * \param curve_in_map The transform defined by the curve location and orientation in the map frame
   * \param p A point in the curve frame
   * \param yaw The orientation of the vehicle at point p in the curve frame
   * 
   * \return A transform where the translation corresponds to point p in the map frame and the rotation corresponds to yaw in the map frame.
   */ 
  Eigen::Isometry2d curvePointInMapTF(const Eigen::Isometry2d& curve_in_map, const lanelet::BasicPoint2d& p, double yaw) const;

  /**
   * \brief Returns the speeds of points closest to the lookahead distance.
   * 
   * \param points The points in the map frame that the trajectory will follow. Units m
   * \param speeds Speeds assigned to points that trajectory will follow. Unit m/s
   * \param lookahead The lookahead distance to obtain future points' speed. Unit m
   * 
   * \return A vector of speed values shifted by the lookahead distance.
   */ 

  std::vector<double> get_lookahead_speed(const std::vector<lanelet::BasicPoint2d>& points, const std::vector<double>& speeds, const double& lookahead);

  /**
   * \brief Computes the lookahead distance based on the input velocity
   * 
   * \param velocity vehicle velocity in m/s.
   * 
   * \return lookahead distance in m.
   */ 
  double get_adaptive_lookahead(double velocity);

private:
  carma_wm::WorldModelConstPtr wm_;
  InLaneCruisingPluginConfig config_;
  PublishPluginDiscoveryCB plugin_discovery_publisher_;

  cav_msgs::Plugin plugin_discovery_msg_;
};
};  // namespace inlanecruising_plugin
