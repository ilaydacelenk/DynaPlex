#pragma once
#include <memory>
#include <string>
#include <span>
#include "state.h"
#include "vargroup.h"
#include "policy.h"
#include "trajectory.h"
namespace DynaPlex
{
	/**
	 * DynaPlex algorithms access MDPs through this interface. Note that you are never required to manually implement this interface, instead
	 * DynaPlex takes a duck-typed specific MDP that adheres to an informal contract
	 * (see examples under models/models.)
	 * and adapts this MDP to implement MDPInterface. 
	 */
	class MDPInterface
	{
	public:
		/**
		 * Returns a unique identifier for this MDP.
		 * The identifier consists of the MDP type id plus a hash of the parameters.
		 * MDPs with the exact same parameters in the same order will have the same identifier.
		 */
		virtual std::string Identifier() const = 0;

		/**
		 * Return an identifier of the underlying type of this MDP. 
		 */
		virtual std::string TypeIdentifier() const = 0;


		/// Provides information about the MDP.
		virtual DynaPlex::VarGroup GetStaticInfo() const = 0;

		/// Retrieves the number of event random number generators used by this MDP
		virtual int64_t NumEventRNGs() const = 0;

		/// Retrieves the discount factor
		virtual double DiscountFactor() const = 0;

		/**
		 * Retrieves the initial state for this MDP.
		 * Note: States are unique for the DynaPlex::MDP instance from which they were created.
		 * Never attempt to call functions on one MDP with states from another MDP,
		 * even if the two MDPs have the same "id".
		 */
		virtual DynaPlex::dp_State GetInitialState() const = 0;

		/// Returns a vector containing actions allowed in the provided state.
		virtual std::vector<int64_t> AllowedActions(const DynaPlex::dp_State&)const = 0;

	
		/**
		 * Incorporates the NextAction into the trajectories. All trajectories in span/vector
		 * must have Category.IsAwaitsAction; throws DynaPlex::Exception otherwise. 
		 * Note: Assumes trajectory.NextAction is allowed for corresponding trajectory.GetState(). 
		 */
		virtual void IncorporateAction(std::span<DynaPlex::Trajectory> trajectories) const  = 0;
		/**
         * Incorporates the action selected by the policy into the trajectories. All trajectories in span/vector
         * must have Category.IsAwaitsAction; throws DynaPlex::Exception otherwise.
		 * Note: Immediately after the call, NextAction for the trajectory still contains the action taken. 
         */
		virtual void IncorporateAction(std::span<DynaPlex::Trajectory> trajectories,const DynaPlex::Policy& policy) const =0;

		/**
		 * Incorporates a single event in each of the trajectories, provided the trajectories are IsAwaitEvent. 
		 * skips any trajectories that were not IsAwaitEvent, and returns a bool indicating whether any states are remaining that require
		 * more events. 
		 */
		virtual bool IncorporateEvent(std::span<DynaPlex::Trajectory> trajectories) const = 0;
		
		//Maybe later, for specific demonstration purposes so that a specific event category can be tested:
		//would require Event to be ConvertibleFromVarGroup. 
	    //virtual void IncorporateEvent(std::span<DynaPlex::Trajectory> trajectories,std::span<DynaPlex::VarGroup> events_as_vargroup) const = 0;

		/**
		 * Incorporates events in the provided trajectories until for each of the provided trajectories at least one of the following holds:
		 * 1. Category IsFinal
		 * 2. EventCount>=MaxEventCount, in which case category will be set to EOH
		 * 3. Category IsAwaitAction, and there is more than a single action allowed
		 * returns true if all states are in category 3, false otherwise. 
         */
		//virtual bool IncorporateUntilNonTrivialAction(std::span<DynaPlex::Trajectory> trajectories,int64_t MaxEventCount) const = 0;

	
		
		/**
		 * Initiates the states in the trajectories. Uses random initial state (GetInitialState(RNG&)) if available, otherwise uses deterministic state (GetInitialState()). 
		 * Updates the Category in the trajectory, and resets EventCount, CumulativeReturn, and EffectiveDiscountFactor. 
		 */
		virtual void InitiateState(std::span<DynaPlex::Trajectory> trajectories) const = 0;
		/**
		 * Sets the states in the trajectories to a specific state value. 
		 * Updates the Category in the trajectory, and resets EventCount, CumulativeReturn, and EffectiveDiscountFactor.
		 */
		virtual void InitiateState(std::span<DynaPlex::Trajectory> trajectories,const DynaPlex::dp_State& state) const = 0;

		/**
		 * Generates a policy specific to this MDP based on the provided VarGroup.
		 * Policies are NOT cross-compatible across MDPs, even generic policies like "random".
		 */
		virtual DynaPlex::Policy GetPolicy(const DynaPlex::VarGroup& vars) const = 0;
		
		/**
		 * Convenience function that calls GetPolicy(VarGroup) with the parameter { "id": id },
		 * i.e., providing only the id. Suitable for "random" and other generic policies,
		 * as well as client-provided policies that need no information beyond the id.
		 * Policies are NOT cross-compatible across MDPs, even generic policies like "random".
		 */
		virtual DynaPlex::Policy GetPolicy(const std::string& id) const = 0;

		/// Lists policies that can be obtained for this MDP
		virtual DynaPlex::VarGroup ListPolicies() const = 0;


		virtual ~MDPInterface() = default;
	};
	using MDP = std::shared_ptr<MDPInterface>;
}
