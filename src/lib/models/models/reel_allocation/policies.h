#pragma once
#include <cstdint>
#include "dynaplex/models/reel_allocation/mdp.h" //#include "mdp.h"
#include "dynaplex/vargroup.h"
#include <memory>

namespace DynaPlex::Models {
	namespace reel_allocation /*must be consistent everywhere for complete mdp defininition and associated policies.*/
	{
		// Forward declaration
		class MDP;


		class IndexPolicyDiscarded3
		{
			//this is the MDP defined inside the current namespace!
			std::shared_ptr<const MDP> mdp;
			const VarGroup varGroup;

			std::vector<int64_t> order_weights;
			std::vector<double> order_probabilities;
			int64_t target;

			// Memoization: unordered_map to store computed values
			std::unordered_map<int64_t, double> memo; // remove muatable

		public:
			IndexPolicyDiscarded3(std::shared_ptr<const MDP> mdp, const VarGroup& config);
			int64_t GetAction(const MDP::State& state) const;
			double FindDiscarded3(int64_t v) const;

		};

	}
}
