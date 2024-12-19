#include "policies.h"
#include "dynaplex/models/reel_allocation2/mdp.h" //#include "mdp.h" //
#include "dynaplex/error.h"
#include <algorithm>
#include <vector>

#include <iostream>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <functional>


namespace DynaPlex::Models {
	namespace reel_allocation2 /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{

		//MDP and State refer to the specific ones defined in current namespace
		IndexPolicyDiscarded3::IndexPolicyDiscarded3(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			//Here, you may initiate any policy parameters.

			order_probabilities = mdp->probabilities;
			order_weights = mdp->WeightOfCompPerType;
			target = mdp->new_material_capacity;

            // Compute W and f_t_now during initialization
            CalculateFT();

		}

        void IndexPolicyDiscarded3::CalculateFT() {
            // Generate valid weights W using backtracking
            W.clear();
            for (int64_t i = 0; i <= target; ++i) {
                W.push_back(i);
            }

            // Initialization
            f_t_now.resize(W.size(), 0.0); // Initialize f_t_now with zeros
            std::vector<double> f_t_prev(W.size(), 0.0); // Vector for previous f_t
            const double threshold = 0.01; // Stopping condition
            double max_min_diff = 500.0; // Initial difference
            int t = 0;

            while (max_min_diff > threshold) {
                // Calculate f_t_now for all values in W
                for (size_t j = 0; j < W.size(); ++j) {
                    int64_t w = W[j];
                    double x_value = 0.0;

                    for (size_t i = 0; i < order_weights.size(); ++i) {
                        int64_t x = order_weights[i];
                        if (w >= x) {
                            // Find index for w - x
                            size_t idx = static_cast<size_t>(w - x);
                            x_value += order_probabilities[i] * f_t_prev[idx];
                        }
                        else {
                            // Find index for target - x
                            size_t idx = static_cast<size_t>(target - x);
                            x_value += order_probabilities[i] * (f_t_prev[idx] + static_cast<double>(w));
                        }
                    }

                    f_t_now[j] = x_value; // Update current value
                }

                // Compute the difference vector
                std::vector<double> diff(W.size());
                for (size_t j = 0; j < W.size(); ++j) {
                    diff[j] = f_t_now[j] - f_t_prev[j];
                }

                // Compute max_min_diff
                double max_diff = *std::max_element(diff.begin(), diff.end());
                double min_diff = *std::min_element(diff.begin(), diff.end());
                max_min_diff = max_diff - min_diff;

                // Print max_min_diff at each iteration
                std::cout << "Iteration " << t << ", max_min_diff: " << max_min_diff << std::endl;
                
                // Print f_t_prev values
                //std::cout << "f_t_prev values at iteration " << t << ": ";
                //for (const auto& val : f_t_prev) {
                //    std::cout << val << " ";
                //}
                //std::cout << std::endl;

                // Update f_t_prev with f_t_now
                f_t_prev = f_t_now;

                // Increment iteration counter
                t += 1;
            }

            // Print the final iteration count
            std::cout << "Final t value: " << t - 1 << std::endl;
        }

        double IndexPolicyDiscarded3::FindDiscarded3(int64_t v) const {
            auto it = std::find(W.begin(), W.end(), v);
            if (it != W.end()) {
                size_t idx = std::distance(W.begin(), it);
                return f_t_now[idx];
            }
            else {
                throw std::invalid_argument("Value not found in W.");
            }
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
					next_expected_discard = FindDiscarded3(mdp->new_material_capacity - state.UpcomingComponentWeight); //+ state.remaining_weight_vector[i]
				}
				

				differences.push_back(next_expected_discard - current_expected_discard); // expected discard for all indices, we prefer the most negative 

			}

			auto minElementIt = std::min_element(differences.begin(), differences.end()); // smallest discard
			int64_t act = std::distance(differences.begin(), minElementIt); // index with smallest expected discard

			return act;
		}

	}
}