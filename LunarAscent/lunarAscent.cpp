/*    Copyright (c) 2010-2018, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#include "lunarAscent.h"

namespace tudat_applications
{
namespace PropagationOptimization2020
{

//! Function that generates thrust acceleration model from thrust parameters
std::shared_ptr< ThrustAccelerationSettings > getThrustAccelerationModelFromParameters(
        const std::vector< double >& decisionVariables,
        const simulation_setup::NamedBodyMap bodyMap,
        const double initialTime,
        const double constantSpecificImpulse )
{
    // Define thrust functions
    std::shared_ptr< LunarAscentThrustGuidance > thrustGuidance =
            std::make_shared< LunarAscentThrustGuidance >(
                bodyMap.at( "Vehicle" ), initialTime, decisionVariables );
    std::function< Eigen::Vector3d( const double ) > thrustDirectionFunction =
            std::bind( &LunarAscentThrustGuidance::getCurrentThrustDirection, thrustGuidance, std::placeholders::_1 );
    std::function< double( const double ) > thrustMagnitudeFunction =
            std::bind( &LunarAscentThrustGuidance::getCurrentThrustMagnitude, thrustGuidance, std::placeholders::_1 );

    std::shared_ptr< ThrustDirectionGuidanceSettings > thrustDirectionGuidanceSettings =
            std::make_shared< CustomThrustDirectionSettings >( thrustDirectionFunction );
    std::shared_ptr< ThrustMagnitudeSettings > thrustMagnitudeSettings =
            std::make_shared< FromFunctionThrustMagnitudeSettings >(
                thrustMagnitudeFunction, [ = ]( const double ){ return constantSpecificImpulse; } );
    return std::make_shared< ThrustAccelerationSettings >(
                thrustDirectionGuidanceSettings, thrustMagnitudeSettings );


}

}

}

using namespace tudat_applications::PropagationOptimization2020;

LunarAscentProblem::LunarAscentProblem( const simulation_setup::NamedBodyMap bodyMap,
                                        const std::shared_ptr< IntegratorSettings< > > integratorSettings,
                                        const std::shared_ptr< MultiTypePropagatorSettings< double > > propagatorSettings,
                                        const std::vector< std::pair< double, double > >& decisionVariableRange,
                                        const double constantSpecificImpulse ):
    bodyMap_(bodyMap), integratorSettings_(integratorSettings), propagatorSettings_(propagatorSettings),
    constantSpecificImpulse_( constantSpecificImpulse )
{
    translationalStatePropagatorSettings_ =
            std::dynamic_pointer_cast< TranslationalStatePropagatorSettings< double > >(
                propagatorSettings_->propagatorSettingsMap_.at( translational_state ).at( 0 ) );


    std::vector< double > boxBoundMinima, boxBoundMaxima;
    for( unsigned int i = 0; i < decisionVariableRange.size( ); i++ )
    {
        boxBoundMinima.push_back( decisionVariableRange.at( i ).first );
        boxBoundMaxima.push_back( decisionVariableRange.at( i ).second );
    }
    boxBounds_ = std::make_pair( boxBoundMinima, boxBoundMaxima );

}

std::vector< double > LunarAscentProblem::fitness( const std::vector< double >& decisionVariables ) const
{
    // Extract existing acceleration settings, and clear existing self-exerted accelerations of vehicle
    simulation_setup::SelectedAccelerationMap accelerationSettings =
            translationalStatePropagatorSettings_->getAccelerationSettingsMap( );
    accelerationSettings[ "Vehicle" ][ "Vehicle" ].clear( );

    // Retrieve new acceleration model for thrust and set in list of settings
    std::shared_ptr< AccelerationSettings > newThrustSettings =
            getThrustAccelerationModelFromParameters(
                decisionVariables, bodyMap_, integratorSettings_->initialTime_, constantSpecificImpulse_ );
    accelerationSettings[ "Vehicle" ][ "Vehicle" ].push_back( newThrustSettings );

    // Update translational propagatot settings
    translationalStatePropagatorSettings_->resetAccelerationModelsMap(
                accelerationSettings, bodyMap_ );

    // Update full propagator settings
    propagatorSettings_->resetIntegratedStateModels( bodyMap_ );
    dynamicsSimulator_ = std::make_shared< SingleArcDynamicsSimulator< > >( bodyMap_, integratorSettings_, propagatorSettings_ );

    computeObjectivesAndConstraints( decisionVariables );

    return objectives_;

}

void LunarAscentProblem::computeObjectivesAndConstraints( const std::vector< double >& decisionVariables ) const
{
    std::map< double, Eigen::VectorXd > stateHistory =
            dynamicsSimulator_->getEquationsOfMotionNumericalSolution( );
    std::map< double, Eigen::VectorXd > dependentVariableHistory =
            dynamicsSimulator_->getDependentVariableHistory( );

    constraints_; // =
    objectives_ = { 0.0 }; // =
}

