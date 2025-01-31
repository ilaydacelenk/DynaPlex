#pragma once
#include <cstdint>
#include "dynaplex/models/reel_allocation2/mdp.h" 
#include "dynaplex/vargroup.h"
#include <memory>
#include <unordered_map>
#include <cstdint>

namespace DynaPlex::Models {
	namespace reel_allocation2 /*must be consistent everywhere for complete mdp defininition and associated policies.*/
	{
		// Forward declaration
		class MDP;


		class IndexPolicyDiscarded3
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;

			std::vector<int64_t> W;           // Valid weights
			std::vector<double> f_t_now;      // Final f_t values corresponding to W

			std::vector<int64_t> order_weights;
			std::vector<double> order_probabilities;
			int64_t target;

			void CalculateFT();               // Private function to compute f_t_now and W

		public:
			IndexPolicyDiscarded3(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			double FindDiscarded3(int64_t v) const;
			int64_t GetAction(const MDP::State& state) const;

		};

	}
}
