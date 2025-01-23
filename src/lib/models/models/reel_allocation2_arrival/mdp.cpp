#include "dynaplex/models/reel_allocation2_arrival/mdp.h" //#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"
#include <algorithm> // For std::sort
#include <vector>

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
			vars.Add("valid_actions", number_of_slots+arrival_size);
			vars.Add("discount_factor", discount_factor);
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
			state.cat = StateCategory::AwaitAction(0);

			// following is added for the initial state, we never see it later
			for (size_t i = 0; i < arrival_size; ++i) {
				if (state.UpcomingComponentWeights[i] == 0) {
					state.UpcomingComponentWeights[i] = event.UpcomingComponentWeight;
				}
			};

			// only the first component will be updated as it was assigned in the prev action
			state.UpcomingComponentWeights[0] = event.UpcomingComponentWeight; // new event ie component request
			

			return 0.0;//we only have costs after an action
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
			

			auto index = state.cat.Index();

			if (!IsAllowedAction(state, action))
			{
				throw DynaPlex::Error("action not allowed: action_index: " + std::to_string(index) + "  action: " + std::to_string(action) );
			}

			if (index == 0) { // if action is to select a component // index 0

				state.cat = StateCategory::AwaitAction(1);//after processing this action, we await a reel selection action.

				std::sort(state.UpcomingComponentWeights.begin(), state.UpcomingComponentWeights.end()); //sorting
				std::swap(state.UpcomingComponentWeights[0], state.UpcomingComponentWeights[action]); // put the component with action index to the first loc

			}
			else { // if action is to select a reel // index 1
				state.cat = StateCategory::AwaitEvent();//after processing this action, we await an event.
				action -= arrival_size;
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
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {

			return (state.cat.Index() == 0 && action < arrival_size) || // 0, 1, 2, ..., K-1
				(state.cat.Index() == 1 && action >= arrival_size && action < number_of_slots + arrival_size); // K, K+1, ... K+N-1

		}

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			//state features as supplied to learning algorithms:
			
			for (auto& comp_weight : state.UpcomingComponentWeights)
			{
				features.Add(comp_weight * 1.0 / new_material_capacity);
			}

			for (auto& weight : state.remaining_weight_vector)
			{
				features.Add(weight * 1.0 / new_material_capacity);
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
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("reel_allocation2_arrival", "A model for material utilization with N active materials and K component requests", registry);
			
		}
	}
}

