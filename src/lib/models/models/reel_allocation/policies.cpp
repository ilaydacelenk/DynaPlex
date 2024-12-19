#include "policies.h"
#include "dynaplex/models/reel_allocation/mdp.h" //#include "mdp.h" //
#include "dynaplex/error.h"
#include <algorithm>
#include <vector>

namespace DynaPlex::Models {
	namespace reel_allocation /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{

		//MDP and State refer to the specific ones defined in current namespace
		IndexPolicyDiscarded3::IndexPolicyDiscarded3(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			//Here, you may initiate any policy parameters.

			order_probabilities = mdp->probabilities;
			order_weights = mdp->WeightOfCompPerType;
			target = mdp->new_material_capacity;

			// find combinations with multiples

			std::vector<std::vector<int>> valid_combinations;  // To store valid combinations
			std::vector<int> current_combination;
			// Backtracking function to find combinations with multiples
			std::function<void(int, int)> backtrack = [&](int current_sum, int start) {
				if (current_sum < target) {
					valid_combinations.push_back(current_combination);
				}
				else {
					return;
				}

				for (size_t i = start; i < order_weights.size(); ++i) {
					int num = order_weights[i];
					int new_sum = current_sum + num;
					if (new_sum < target) {
						current_combination.push_back(num);
						backtrack(new_sum, i);  // Keep the same index to allow repeats
						current_combination.pop_back();  // Backtrack
					}
				}
				};

			// Start the backtracking process
			backtrack(0, 0);

			// Calculate the possible weights
			std::vector<int> possible_weights;
			for (const auto& comb : valid_combinations) {
				int sum_of_comb = 0;
				for (int num : comb) {
					sum_of_comb += num;
				}
				possible_weights.push_back(target - sum_of_comb);
			}

			// Sort the possible weights before returning
			sort(possible_weights.begin(), possible_weights.end());

			//fill memo here, compute all
			for (size_t i = 0; i < possible_weights.size(); ++i) {
				int64_t v = possible_weights[i];
				double result = 0.0;

				// Precompute result for this specific `v`
				for (size_t j = 0; j < order_weights.size(); ++j) {
					int64_t w = order_weights[j];
					if (w <= v) {
						result += order_probabilities[j] * FindDiscarded3(v - w); // Recursive call
					}
					else {
						result += order_probabilities[j] * v; // Base case when no fitting weight
					}
				}

				// Store the precomputed result in memo
				memo[v] = result;
			}

			//for (const auto& pair : memo) {
			//	std::cout << "Key (v): " << pair.first << ", Value: " << pair.second << std::endl;
			//}

		}

		double IndexPolicyDiscarded3::FindDiscarded3(int64_t v) const {

			// Check if the result for `v` is already computed and stored in memo
			if (memo.find(v) != memo.end()) {
				return memo.at(v);
				//return memo[v];  // Return cached result
			}

			double result = 0.0;

			for (size_t i = 0; i < order_weights.size(); ++i) {
				int64_t w = order_weights[i];
				if (w <= v) { //can fit
					result += order_probabilities[i] * FindDiscarded3(v - w);
				}
				else { // cannot fit
					result += order_probabilities[i] * v; // for base cases
				}

			}

			return result;
		}

		int64_t IndexPolicyDiscarded3::GetAction(const MDP::State& state) const
		{
			//Implement custom policy, and remove below line.
			//throw DynaPlex::NotImplementedError();

			// for i in 0 to number of slots-1, compute expected discard, choose the action i with the smallest 

			//std::vector<int64_t> weights = mdp->WeightOfCompPerType;

			double current_expected_discard;
			double next_expected_discard;
			std::vector<double> differences;
			for (int i = 0; i < mdp->number_of_slots; i++)
			{
				current_expected_discard = FindDiscarded3(state.remaining_weight_vector[i]);

				if (state.remaining_weight_vector[i] - state.UpcomingComponentWeight >= 0) { // can fit
					next_expected_discard = FindDiscarded3(state.remaining_weight_vector[i] - state.UpcomingComponentWeight);
				}
				else { // cannot fit
					next_expected_discard = FindDiscarded3(mdp->new_material_capacity - state.UpcomingComponentWeight) + state.remaining_weight_vector[i]; 
					//next_expected_discard = FindDiscarded3(mdp->new_material_capacity) + state.remaining_weight_vector[i];
					// F(new) + loss in the current periods
				}
				

				differences.push_back(next_expected_discard - current_expected_discard); // expected discard for all indices, we prefer the most negative 

			}

			auto minElementIt = std::min_element(differences.begin(), differences.end()); // smallest discard
			int64_t act = std::distance(differences.begin(), minElementIt); // index with smallest expected discard

			return act;
		}

	}
}