/*
 * Copyright (C) 2017 LEIDOS.
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

package gov.dot.fhwa.saxton.carma.guidance.maneuvers;

import gov.dot.fhwa.saxton.carma.guidance.IGuidanceCommands;

/**
 * Base class for all maneuver objects.
 */
public abstract class ManeuverBase implements IManeuver {

    protected double                            startDist_ = -1.0;
    protected double                            endDist_ = -1.0;
    protected IManeuverInputs                   inputs_;
    protected IGuidanceCommands                 commands_;

    /**
     * Provides the common planning capability that all maneuvers will need. Concrete maneuver classes
     * will need to provide their own plan() methods to fill in the details and execute this one first.
     */
    @Override
    public void plan(IManeuverInputs inputs, IGuidanceCommands commands, double startDist) throws IllegalStateException {
        inputs_ = inputs;
        commands_ = commands;
        startDist_ = startDist;
    }

    /**
     * Provides the common planning capability that all maneuvers will need. Concrete maneuver classes
     * will need to provide their own plan() methods to fill in the details and execute this one first.
     */
    @Override
    public void planToTargetDistance(IManeuverInputs inputs, IGuidanceCommands commands, double startDist, double endDist) throws IllegalStateException {
        inputs_ = inputs;
        commands_ = commands;
        startDist_ = startDist;
    }

    public abstract boolean executeTimeStep() throws IllegalStateException;


    public abstract void setSpeeds(double startSpeed, double targetSpeed) throws UnsupportedOperationException;


    public abstract void setTargetLane(int targetLane) throws UnsupportedOperationException;


    @Override
    public double getStartDistance() {
        return startDist_;
    }


    @Override
    public double getEndDistance() {
        return endDist_;
    }


    /**
     * Verifies that the vehicle is between the specified start & end locations for this maneuver
     * @throws IllegalStateException if we are outside the allowable region
     */
    protected void verifyLocation() throws IllegalStateException {
        double currentLocation = inputs_.getDistanceFromRouteStart();
        if (currentLocation < startDist_  ||  currentLocation > endDist_) {
            throw new IllegalStateException("Maneuver attempted to execute at distance " + currentLocation
                                            + ". Maneuver start dist = " + startDist_ + ", end dist = " + endDist_);
        }
    }
}