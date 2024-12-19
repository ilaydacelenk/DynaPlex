#include "policies.h"
#include "mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace airplane /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{

		//MDP and State refer to the specific ones defined in current namespace
		RuleBasedPolicy::RuleBasedPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			//provide default values and load parameters from json
			config.GetOrDefault("price_threshold_low", price_threshold_low, 0.0);
			config.GetOrDefault("price_threshold_high", price_threshold_high, 1.0);
			//we can also set parameters directly in the constructor
			seat_threshold_low = 1;
			seat_threshold_high = 5;
            remainingday_threshold = 9;
		}


		int64_t RuleBasedPolicy::GetAction(const MDP::State& state) const
        {
            if (state.RemainingSeats > seat_threshold_high)
            {
                return 1;//sell
            }
            if (state.PriceOfferedPerSeat > price_threshold_low)
            {//only sell to type 1 and 2 customers
                if (state.RemainingSeats <= seat_threshold_high && state.RemainingSeats >= seat_threshold_low)
                {
                    if (state.RemainingDays <= remainingday_threshold)
                    {
                        return 1;//sell
                    }

                }
            }
            if (state.PriceOfferedPerSeat > price_threshold_high)
            {//only sell to type 1 customers
                if (state.RemainingSeats <= seat_threshold_high && state.RemainingSeats >= seat_threshold_low)
                {
                    if (state.RemainingDays > remainingday_threshold)
                    {
                        return 1;//sell
                    }

                }
            }
            return 0;//no sales
        }
	}
}