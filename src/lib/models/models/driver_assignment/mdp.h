#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"

namespace DynaPlex::Models {
	namespace driver_assignment /*must be consistent everywhere for complete mdp definition and associated policies and states (if not defined inline).*/
	{
		class MDP
		{
		public:	// costlar eklencek
			int64_t number_drivers;
			double mean_order_per_time_unit;

			double max_x;
			double min_x = 0;
			double max_y;
			double min_y = 0;

			double max_working_hours;
			double horizon_length;

			double cost_per_km;
			double penalty_for_workhour_violation;
			//double	timewindow_duration;

			struct State {
				//using this is recommended:
				DynaPlex::StateCategory cat;
				//driver state attributes: lokasyon_X, lokasyon_y, busy/idle, remaining work hours
				std::vector<double> current_driver_list_conc;///bunu nerede tutacagimizi tarkana sorcaz!!!!
				//order state attributes: type (pick_xy, deliver_xy-4)
				std::vector<double> current_order_typelist;

				double current_time;
				DynaPlex::VarGroup ToVarGroup() const;
			};
			using Event = std::vector<double>;

			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event&) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			//std::vector<std::tuple<Event,double>> EventProbabilities() const; exactsolverda kullanilir(optimal cozum icin), bu problem icin gerek yok
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;
			State GetInitialState(DynaPlex::RNG& rng) const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}
