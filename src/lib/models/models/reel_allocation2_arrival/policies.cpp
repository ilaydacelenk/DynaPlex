#include "policies.h"
#include "dynaplex/models/reel_allocation2_arrival/mdp.h" //#include "mdp.h" //
#include "dynaplex/error.h"
#include <algorithm>
#include <vector>
#include <deque>

#include <iostream>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <functional>


namespace DynaPlex::Models {
	namespace reel_allocation2_arrival /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
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
            std::vector<double> f_t_prev(W.size(), 0.0); // Initialize vector for previous f_t for t=0
            const double epsilon = 0.0001; // Stopping condition
            double m_t = 0.0; // Initialize min difference
            double M_t = 5000.0; // Initialize max difference
            int t = 0;

            while ( M_t - m_t > epsilon * m_t ) { //(M_t - m_t > epsilon * m_t) && (t < 10)
                // Increment iteration counter
                t += 1;

                // Calculate f_t_now for all values in W
                for (size_t j = 0; j < W.size(); ++j) {
                    int64_t w = W[j];
                    double f_value = 0.0;

                    for (size_t i = 0; i < order_weights.size(); ++i) {
                        int64_t x = order_weights[i];
                        if (w >= x) {
                            // Find index for w - x
                            size_t idx = static_cast<size_t>(w - x);
                            f_value += order_probabilities[i] * f_t_prev[idx];
                        }
                        else {
                            // Find index for target - x
                            size_t idx = static_cast<size_t>(target - x);
                            f_value += order_probabilities[i] * (f_t_prev[idx] + static_cast<double>(w));
                        }
                    }

                    f_t_now[j] = f_value; // Update current value
                }

                // Compute the difference vector
                std::vector<double> diff(W.size());
                for (size_t j = 0; j < W.size(); ++j) {
                    diff[j] = f_t_now[j] - f_t_prev[j];
                };

                // Compute max_min_diff
                M_t = *std::max_element(diff.begin(), diff.end());
                m_t = *std::min_element(diff.begin(), diff.end());
                //max_min_diff = max_diff - min_diff;

                // Print max_min_diff at each iteration
                std::cout << "Iteration " << t << ", max_min_diff: " << M_t - m_t << std::endl;
                
                // Print f_t_prev values
                //std::cout << "f_t_prev values at iteration " << t << ": ";
                //for (const auto& val : f_t_prev) {
                //    std::cout << val << " ";
                //}
                //std::cout << std::endl;

                
                f_t_prev = f_t_now; // Update f_t_prev with f_t_now

                
            }
            double min_f_t = *std::min_element(f_t_now.begin(), f_t_now.end());
            // Subtract min_f_t from each element of f_t_prev, relative f
            for (size_t i = 0; i < f_t_prev.size(); ++i) {
                f_t_now[i] = f_t_prev[i] - min_f_t;
            }

            // Print the final iteration count
            std::cout << "Final t value: " << t << std::endl;
        }

        double IndexPolicyDiscarded3::FindDiscarded3(int64_t v) const {
            auto it = std::find(W.begin(), W.end(), v);
            size_t idx = std::distance(W.begin(), it);
            return f_t_now[idx];
        }

		int64_t IndexPolicyDiscarded3::GetAction(const MDP::State& state) const
		{
			//Implement custom policy, and remove below line.
			//throw DynaPlex::NotImplementedError();

			// for i in 0 to number of slots-1, compute expected discard, choose the action i with the smallest 

			//std::vector<int64_t> weights = mdp->WeightOfCompPerType;

            auto index = state.cat.Index();

                   
            //std::vector<std::vector<double>> differences_all; 

            std::vector<double> min_differences;
            double current_expected_discard;
            double next_expected_discard;
            std::vector<double> differences;

            if (index == 0) { // if action is to select a component // index 0
                
                for (int j = 0; j < mdp->arrival_size; j++) {

                    differences.clear();
                    for (int i = 0; i < mdp->number_of_slots; i++)
                    {
                        current_expected_discard = FindDiscarded3(state.remaining_weight_vector[i]);

                        if (state.remaining_weight_vector[i] - state.UpcomingComponentWeights[j] >= 0) { // can fit
                            next_expected_discard = FindDiscarded3(state.remaining_weight_vector[i] - state.UpcomingComponentWeights[j]);
                        }
                        else { // cannot fit
                            next_expected_discard = FindDiscarded3(mdp->new_material_capacity - state.UpcomingComponentWeights[j]) + state.remaining_weight_vector[i];
                        }

                        differences.push_back(next_expected_discard - current_expected_discard); // expected discard for all reels for specific comp
                        
                    };
                    min_differences.push_back(*std::min_element(differences.begin(), differences.end()));
                    
                }

                auto minElementIt = std::min_element(min_differences.begin(), min_differences.end()); // smallest discard for each comp
                int64_t act = std::distance(min_differences.begin(), minElementIt); // index of comp with smallest expected discard
                
                return act;
            }
            else { // if action is to select a reel // index 1

                differences.clear();
                // random component assign etseydim change in expected discard nasil olurdu
                for (int i = 0; i < mdp->number_of_slots; i++)
                {
                    current_expected_discard = FindDiscarded3(state.remaining_weight_vector[i]);

                    if (state.remaining_weight_vector[i] - state.UpcomingComponentWeights[0] >= 0) { // can fit
                        next_expected_discard = FindDiscarded3(state.remaining_weight_vector[i] - state.UpcomingComponentWeights[0]);
                    }
                    else { // cannot fit
                        next_expected_discard = FindDiscarded3(mdp->new_material_capacity - state.UpcomingComponentWeights[0]) + state.remaining_weight_vector[i];
                    }

                    differences.push_back(next_expected_discard - current_expected_discard); // expected discard for all indices, we prefer the most negative 

                }
                auto minElementIt = std::min_element(differences.begin(), differences.end()); // smallest discard
                int64_t act = std::distance(differences.begin(), minElementIt) + mdp->arrival_size; // index with smallest expected discard

                return act;
            }

			
		}

	}
}