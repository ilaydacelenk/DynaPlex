#include "dynaplex/models/reel_allocation2_arrival/mdp.h" //#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"
#include <algorithm> // For std::sort


namespace DynaPlex::Models {
	namespace reel_allocation2_arrival /*keep this in line with id below and with namespace name in header*/
	{
		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			//add all state variables
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("remaining_weight_vector", remaining_weight_vector);
			vars.Add("UpcomingComponentWeights", UpcomingComponentWeights);

			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("remaining_weight_vector", state.remaining_weight_vector);
			vars.Get("UpcomingComponentWeights", state.UpcomingComponentWeights);
			return state;
		}


		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			vars.Add("valid_actions", number_of_slots);
			vars.Add("discount_factor", discount_factor);

			//This indicates that the MDP never terminates. 
			//It may be used by various algorithms. 
			//infinite is default, other value is finite for algorithms that are guaranteed to reach a final state at some point. 
			vars.Add("horizon_type", "infinite");


			return vars;
		}

		MDP::MDP(const VarGroup& config)
		{
			config.Get("new_material_capacity", new_material_capacity);
			config.Get("number_of_slots", number_of_slots);
			config.Get("arrival_size", arrival_size);
			config.Get("WeightOfCompPerType", WeightOfCompPerType);
			config.Get("comp_dist", comp_dist);
			config.Get("probabilities", probabilities);

			if (config.HasKey("discount_factor"))
				config.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;

		}

		MDP::State MDP::GetInitialState() const
		{
			State state{};
			state.cat = StateCategory::AwaitEvent();//or AwaitAction(), depending on logic
			//initiate other variables.
			state.UpcomingComponentWeights = std::vector<int64_t>(arrival_size, 0);
			state.remaining_weight_vector = std::vector<int64_t>(number_of_slots, 0); 
			return state;
		}

		MDP::Event MDP::GetEvent(RNG& rng) const {
			//generate an event using the custom discrete distribution (see MDP() initializer)
			int64_t compType = comp_dist.GetSample(rng);
			int64_t weight = WeightOfCompPerType.at(compType);
			return Event(weight);//return the weight of the component type
		}


		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			//after processing this event, we await an action.
			state.cat = StateCategory::AwaitAction();
			for (size_t i = 0; i < arrival_size; ++i) {
				if (state.UpcomingComponentWeights[i] == 0) {
					state.UpcomingComponentWeights[i] = event.UpcomingComponentWeight;
				}
			};
			return 0.0;//we only have costs after an action
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
	
			state.cat = StateCategory::AwaitEvent();//after processing this action, we await an event.
		
			int64_t diff = state.remaining_weight_vector[action] - state.UpcomingComponentWeights[0];

			if (diff >= 0)//it can fit
			{
				state.remaining_weight_vector[action] -= state.UpcomingComponentWeights[0]; // reduce the material weight by component weight
				std::sort(state.remaining_weight_vector.begin(), state.remaining_weight_vector.end()); //sorting
				return 0.0;
			}
			else//cannot fit
			{
				state.remaining_weight_vector[action] = new_material_capacity - state.UpcomingComponentWeights[0]; // use a new material
				std::sort(state.remaining_weight_vector.begin(), state.remaining_weight_vector.end()); //sorting
				return diff + state.UpcomingComponentWeights[0]; //discard the previous material, ie previous state.remaining_weight_vector[action]
			}
			
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			return true;

		}

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			//state features as supplied to learning algorithms:
			
			
			features.Add(1.0*state.UpcomingComponentWeights[0] / new_material_capacity);
			
			for (auto& weight : state.remaining_weight_vector)
			{
				features.Add(weight * 1.0 / new_material_capacity);
			}

			for (auto& weight : state.remaining_weight_vector)
			{
				if (weight - state.UpcomingComponentWeights[0] >= 0) { //if can fit
					features.Add((weight - state.UpcomingComponentWeights[0]) * 1.0 / new_material_capacity);
				}
				else { // cannot fit
					features.Add((new_material_capacity - state.UpcomingComponentWeights[0]) * 1.0 / new_material_capacity);
				}
				
			}
		}




		std::vector<std::tuple<MDP::Event, double>> MDP::EventProbabilities() const
		{
			//This is optional to implement. You only need to implement it if you intend to solve versions of your problem
			//using exact methods that need access to the exact event probabilities.
			//Note that this is typically only feasible if the state space if finite and not too big, i.e. at most a few million states.
			throw DynaPlex::NotImplementedError();
		}


		
		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			//no custom policies registered currently for this MDP. 
			registry.Register<IndexPolicyDiscarded3>("index_policy_discarded3", "expected discard based");
		}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			//this typically works, but state.cat must be kept up-to-date when modifying states. 
			return state.cat;
		}



		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("reel_allocation2_arrival", "A model for material utilization with a # of active materials and a # of arrivals", registry);
			
		}
	}
}

