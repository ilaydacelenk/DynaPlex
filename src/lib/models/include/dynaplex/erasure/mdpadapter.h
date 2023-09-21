#pragma once
#include <vector>
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "dynaplex/rng.h"
#include "dynaplex/mdp.h"
#include "dynaplex/state.h"
#include "dynaplex/policy.h"

#include "erasure_concepts.h"
#include "randompolicy.h"
#include "policyregistry.h"
#include "stateadapter.h"


namespace DynaPlex::Erasure
{
	template<typename t_MDP>
	class MDPAdapter final : public MDPInterface
	{
		static_assert(DynaPlex::Concepts::ConvertibleFromVarGroup<t_MDP>, "MDP must define public constructor with const VarGroup& parameter");
		static_assert(HasState<t_MDP>, "MDP must publicly define a nested type or using declaration for State");
		static_assert(HasGetStaticInfo<t_MDP>, "MDP must publicly define GetStaticInfo() const returning DynaPlex::VarGroup.");
		using t_State = typename t_MDP::State;
		using t_Event = std::conditional_t<HasEvent<t_MDP>, typename t_MDP::Event, int64_t>;
		
		//Should we require this right off the bat and be done with it?
		//static_assert(DynaPlex::Concepts::ConvertibleToVarGroup<t_State>, "MDP::State must define VarGroup ToVarGroup() const");
		
		std::string unique_id;
		int64_t mdp_int_hash;
		std::shared_ptr<const t_MDP> mdp;
		std::string mdp_type_id;
		PolicyRegistry<t_MDP> policy_registry;
		ActionRangeProvider<t_MDP> provider;
		double discount_factor;

		//Registers the policies with the internal registry:
		void RegisterPolicies()
		{
			//Register built-in policies
			policy_registry.template Register<RandomPolicy<t_MDP>>("random", "makes a random choice between the allowed actions");
			//register client-provided policies. 
			if constexpr (HasRegisterPolicies<t_MDP, PolicyRegistry<t_MDP>>) {
				mdp->RegisterPolicies(policy_registry);
			}
		}

		void InitializeVariables()
		{
			auto static_vars = GetStaticInfo();
			if (static_vars.HasKey("discount_factor"))
				static_vars.Get("discount_factor", discount_factor);

			if (discount_factor > 1.0 || discount_factor <= 0.0)
			{
				throw DynaPlex::Error("MDP, id \"" + mdp_type_id + "\" : discount_factor is invalid: " + std::to_string(discount_factor) + ". Must be in (0.0,1.0].");
			}
		}

		

	public:
		MDPAdapter(const DynaPlex::VarGroup& config) :
			mdp{ std::make_shared<const t_MDP>(config) },
			unique_id{ config.UniqueIdentifier() },
			mdp_int_hash{ config.Int64Hash() },
			mdp_type_id{ config.Identifier() },
			policy_registry{},
			provider{ mdp },
			discount_factor{ 1.0 }
		{
			InitializeVariables();
			RegisterPolicies();
		}		

		double DiscountFactor() const override
		{
			return discount_factor;
		}

		int64_t NumEventRNGs() const override
		{
			//At present, multiple event streams not supported.
			return 1;
		}

		//Defined in mdpadapter_tostate.h included below
		const t_State& ToState(const DynaPlex::dp_State& state) const;		
		t_State& ToState(DynaPlex::dp_State& state) const;

		std::vector<int64_t> AllowedActions(const DynaPlex::dp_State& dp_state) const override
		{
			auto& state = ToState(dp_state);	
			auto actions = provider(state);
			std::vector<int64_t> vec;
			vec.reserve(actions.Count());
			for (int64_t action : actions)
			{
				vec.push_back(action);
			}			
			return vec;
		}

		std::string TypeIdentifier() const override
		{
			return mdp_type_id;
		}

		std::string Identifier() const override
		{
			return unique_id;
		}

		DynaPlex::VarGroup GetStaticInfo() const override
		{	//guaranteed to exist by static_assert:
			return mdp->GetStaticInfo();
		}

		DynaPlex::dp_State GetInitialState() const override
		{
			if constexpr (HasGetInitialState<t_MDP>)
			{
				t_State state = mdp->GetInitialState();
				//adding hash to facilitate identifying this state as coming from current MDP later on.
				return std::make_unique<StateAdapter<t_State>>(mdp_int_hash, state);
			}
			else
				throw DynaPlex::Error("MDP.GetInitialStateVec in MDP: " + mdp_type_id + "\nMDP must publicly define GetInitialState() const returning MDP::State.");
		}
		
		

		void IncorporateAction(std::span<DynaPlex::Trajectory> trajectories) const override
		{
			if constexpr (HasModifyStateWithAction<t_MDP>)
			{
				for (DynaPlex::Trajectory& traj : trajectories)
				{
					auto& state = ToState(traj.GetState());
					
					if (traj.Category.IsAwaitAction())
					{
						traj.CumulativeReturn += mdp->ModifyStateWithAction(state, traj.NextAction) * traj.EffectiveDiscountFactor;
						traj.Category = mdp->GetStateCategory(state);
					}
					else
					{
						throw DynaPlex::Error("MDP.IncorporateActions: Cannot incorporate action if Trajectory.Category is not AwaitAction");
					}
				}
			}
			else
				throw DynaPlex::Error("MDP.IncorporateActions: " + mdp_type_id + "\nMDP does not publicly define ModifyStateWithAction(MDP::State,int64_t) const returning double");
			
		}
		void IncorporateAction(std::span<DynaPlex::Trajectory> trajectories, const DynaPlex::Policy& policy) const override
		{
			policy->SetAction(trajectories);
			IncorporateAction(trajectories);
		}

		
		bool IncorporateEvent(std::span<DynaPlex::Trajectory> trajectories) const override
		{

			bool EventsRemaining = false;
			for (DynaPlex::Trajectory& traj : trajectories)
			{
				if (traj.Category.IsAwaitEvent())
				{
					auto& state = ToState(traj.GetState());
					traj.EventCount++;
					traj.EffectiveDiscountFactor *= discount_factor;
					auto event_stream = traj.Category.Index();
					if constexpr (HasGetEvent<t_MDP, t_Event, DynaPlex::RNG> && HasModifyStateWithEvent<t_MDP, t_State, t_Event>)
					{
						t_Event Event = mdp->GetEvent(traj.RNGProvider.GetEventRNG(event_stream));
						traj.CumulativeReturn += mdp->ModifyStateWithEvent(state, Event) * traj.EffectiveDiscountFactor;
					}
					else if constexpr (HasModifyStateWithEventRNG<t_MDP, t_State, DynaPlex::RNG>)
					{
						traj.CumulativeReturn += mdp->ModifyStateWithEvent(state, traj.RNGProvider.GetEventRNG(event_stream)) * traj.EffectiveDiscountFactor;
					}
					else //if constexpr 
						throw DynaPlex::Error("MDP.IncorporateEvent: " + mdp_type_id + "\nMDP does not publicly define GetEvent(DynaPlex::RNG&) const returning MDP::Event and ModifyStateWithEvent(MDP::State&, const MDP::Event&) returning double.");


					traj.Category = mdp->GetStateCategory(state);
					if (traj.Category.IsAwaitEvent())
					{
						EventsRemaining = true;
					}
				}
			}
			return EventsRemaining;
		}
		virtual void InitiateState(std::span<DynaPlex::Trajectory> trajectories) const override
		{		
			if constexpr (HasGetInitialRandomState<t_MDP, DynaPlex::RNG>)
			{
				for (DynaPlex::Trajectory& traj : trajectories)
				{
					t_State state = mdp->GetInitialState(traj.RNGProvider.GetInitiationRNG());
					traj.Category = mdp->GetStateCategory(state);
					traj.Reset(std::move(std::make_unique<StateAdapter<t_State>>(mdp_int_hash, state)));
				}
			}
			else if constexpr (HasGetInitialState<t_MDP>)
			{
				for (DynaPlex::Trajectory& traj : trajectories)
				{
					t_State state = mdp->GetInitialState();
					traj.Category = mdp->GetStateCategory(state);
					traj.Reset(std::move( std::make_unique<StateAdapter<t_State>>(mdp_int_hash, state)));
				}
			}
			else
				throw DynaPlex::Error("MDP.InitiateState: " + mdp_type_id + "\nMDP does not publicly define GetInitialState() const or GetInitialState(DynaPlex::RNG&) const returning MDP::State");
		}
		virtual void InitiateState(std::span<DynaPlex::Trajectory> trajectories, const DynaPlex::dp_State& state) const override
		{
			for (DynaPlex::Trajectory& traj : trajectories)
			{
				traj.Reset(std::move(state->Clone()));
				auto& t_state = ToState(state);
				traj.Category = mdp->GetStateCategory(t_state);
			}
		}



		DynaPlex::VarGroup ListPolicies() const override
		{
			return policy_registry.ListPolicies();
		}

		DynaPlex::Policy GetPolicy(const std::string& id) const override
		{
			DynaPlex::VarGroup vars{};
			vars.Add("id", id);
			return GetPolicy(vars);
		}

		DynaPlex::Policy GetPolicy(const DynaPlex::VarGroup& varGroup) const override
		{
			return policy_registry.GetPolicy(mdp, varGroup, mdp_int_hash);
		}

	};

}

#include "mdpadapter_tostate.h"



	