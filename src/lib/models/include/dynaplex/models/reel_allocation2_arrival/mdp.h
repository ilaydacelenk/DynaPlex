#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include <vector>

namespace DynaPlex::Models {
	namespace reel_allocation2_arrival /*must be consistent everywhere for complete mdp definition and associated policies and states (if not defined inline).*/
	{		
		class MDP
		{
			double discount_factor;
			DynaPlex::DiscreteDist comp_dist;//the distribution of component types			

		public:	
			int64_t new_material_capacity; //previously defined above, but want to access it in policy so it needs to be public
			int64_t number_of_slots; //previously defined above, but wanted to access it in policy so it needs to be public
			int64_t arrival_size;
			std::vector<int64_t> WeightOfCompPerType;//used in policy
			std::vector<double> probabilities;//the distribution of component types


			struct State {
				DynaPlex::StateCategory cat;
				std::vector<int64_t> remaining_weight_vector;
				std::vector<int64_t> UpcomingComponentWeights;

				DynaPlex::VarGroup ToVarGroup() const;

				//Defaulting this does not always work. It can be removed as only the exact solver would benefit from this.
				bool operator==(const State& other) const = default;
			};
			struct Event {
				int64_t UpcomingComponentWeight;
			};

			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event& ) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			std::vector<std::tuple<Event,double>> EventProbabilities() const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

