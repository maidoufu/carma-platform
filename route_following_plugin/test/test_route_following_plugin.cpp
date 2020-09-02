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

#include "route_following_plugin.h"
#include <gtest/gtest.h>
#include <ros/ros.h>
#include <thread>
#include <chrono>

namespace route_following_plugin
{

    TEST(RouteFollowingPluginTest, testFindLaneletIndexFromPath)
    {
        RouteFollowingPlugin rfp;
        lanelet::ConstLanelets lls;
        lanelet::Lanelet ll;
        ll.setId(15);
        lls.push_back(ll);
        lanelet::routing::LaneletPath path(lls);
        EXPECT_EQ(0, rfp.findLaneletIndexFromPath(15, path));
        EXPECT_EQ(-1, rfp.findLaneletIndexFromPath(5, path));
    }

    TEST(RouteFollowingPluginTest, testComposeManeuverMessage)
    {
        RouteFollowingPlugin rfp;
        ros::Time current_time=ros::Time(0,0);
        auto msg = rfp.composeManeuverMessage(1.0, 10.0, 0.9, RouteFollowingPlugin::TWENTY_FIVE_MPH_IN_MS, 2, current_time);
        EXPECT_EQ(cav_msgs::Maneuver::LANE_FOLLOWING, msg.type);
        EXPECT_EQ(cav_msgs::ManeuverParameters::NO_NEGOTIATION, msg.lane_following_maneuver.parameters.neogition_type);
        EXPECT_EQ(cav_msgs::ManeuverParameters::HAS_TACTICAL_PLUGIN, msg.lane_following_maneuver.parameters.presence_vector);
        EXPECT_EQ("InlaneCruisingPlugin", msg.lane_following_maneuver.parameters.planning_tactical_plugin);
        EXPECT_EQ("RouteFollowingPlugin", msg.lane_following_maneuver.parameters.planning_strategic_plugin);
        EXPECT_NEAR(1.0, msg.lane_following_maneuver.start_dist, 0.01);
        EXPECT_NEAR(0.9, msg.lane_following_maneuver.start_speed, 0.01);
        EXPECT_EQ(ros::Time(0), msg.lane_following_maneuver.start_time);
        EXPECT_NEAR(10.0, msg.lane_following_maneuver.end_dist, 0.01);
        EXPECT_NEAR(25 / 2.237, msg.lane_following_maneuver.end_speed, 0.01);
        EXPECT_TRUE(msg.lane_following_maneuver.end_time - ros::Time(1.49) < ros::Duration(0.01));
        EXPECT_EQ("2", msg.lane_following_maneuver.lane_id);
    }


    TEST(RouteFollowingPluginTest,test_LaneChange)
    {
        RouteFollowingPlugin rfp;
        auto relations=lanelet::routing::LaneletRelations();
        lanelet::routing::LaneletRelation relation1;
        relations.push_back(relation1);
        lanelet::routing::LaneletRelation relation2;
        relation2.relationType=lanelet::routing::RelationType::Successor;
        
        //identifyLaneChange checks for all relations and returns true is whenever relation is Successor
        for(size_t i=0;i<2;i++){
            if(rfp.identifyLaneChange(relations, 0)) 
            {
                EXPECT_TRUE(i==1);
                //std::cout<<"Changing Lanes"<<std::endl;
                //Generate lane change message
                ros::Time current_time=ros::Time(0,0);
                auto msg = rfp.composeManeuverMessage_lanechange(1.0, 10.0, 0.9, RouteFollowingPlugin::TWENTY_FIVE_MPH_IN_MS, 2,3, current_time);
                EXPECT_EQ(cav_msgs::Maneuver::LANE_CHANGE, msg.type);
                EXPECT_EQ(cav_msgs::ManeuverParameters::NO_NEGOTIATION, msg.lane_change_maneuver.parameters.neogition_type);
                EXPECT_EQ(cav_msgs::ManeuverParameters::HAS_TACTICAL_PLUGIN, msg.lane_change_maneuver.parameters.presence_vector);
                EXPECT_EQ("unobstructed_lanechange", msg.lane_change_maneuver.parameters.planning_tactical_plugin);
                EXPECT_EQ("RouteFollowingPlugin", msg.lane_change_maneuver.parameters.planning_strategic_plugin);
                EXPECT_NEAR(1.0, msg.lane_change_maneuver.start_dist, 0.01);
                EXPECT_NEAR(0.9, msg.lane_change_maneuver.start_speed, 0.01);
                EXPECT_EQ(ros::Time(0), msg.lane_change_maneuver.start_time);
                EXPECT_NEAR(10.0, msg.lane_change_maneuver.end_dist, 0.01);
                EXPECT_NEAR(25 / 2.237, msg.lane_change_maneuver.end_speed, 0.01);
                EXPECT_TRUE(msg.lane_change_maneuver.end_time - ros::Time(1.73) < ros::Duration(0.01));
                EXPECT_EQ("2", msg.lane_change_maneuver.starting_lane_id);
                EXPECT_EQ("3", msg.lane_change_maneuver.ending_lane_id);
            }
            else    
            {
                EXPECT_TRUE(i==0);
                //std::cout<<"Staying in same Lane"<<std::endl;
            }
            relations.pop_back();
            relations.push_back(relation2);
        }

    }

    TEST(RouteFollowingPluginTest, testIdentifyLaneChange)
    {
        RouteFollowingPlugin rfp;
        auto relations = lanelet::routing::LaneletRelations();
        EXPECT_FALSE(rfp.identifyLaneChange(relations, 1));
        lanelet::routing::LaneletRelation relation;
        relation.relationType = lanelet::routing::RelationType::Successor;
        relations.push_back(relation);
        EXPECT_TRUE(rfp.identifyLaneChange(relations, 0));
    }

}

// Run all the tests
int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    auto res = RUN_ALL_TESTS();
    return res;
}


