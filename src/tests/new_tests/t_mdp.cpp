﻿#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/Demonstrator.h"
namespace DynaPlex::Tests {
	
	void ExecuteTest(const std::string& model_name, const std::string& config_name)
	{
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.GetSystem();


		ASSERT_TRUE(
			system.file_exists("defaults", model_name, config_name)
		);

		std::string file_path = system.filename("defaults", model_name, config_name);

		DynaPlex::VarGroup vars = VarGroup::LoadFromFile(file_path);

		DynaPlex::MDP mdp;
		DynaPlex::Policy policy;

		ASSERT_NO_THROW(
			mdp = dp.GetMDP(vars);
		);
		ASSERT_NO_THROW(
			policy = mdp->GetPolicy("random");
		);

		int64_t numEventTrajectories;
		ASSERT_NO_THROW(
			numEventTrajectories = mdp->NumEventRNGs();
		);
		Trajectory trajectory{ numEventTrajectories };



		ASSERT_NO_THROW(
		);
		ASSERT_NO_THROW(
			mdp->InitiateState({ &trajectory,1 });
		);
		ASSERT_NO_THROW(
			trajectory.SeedRNGProvider(dp.GetSystem(), true, 123);
		);

		int64_t max_event_count = 10;
		bool finalreached = false;
		while (trajectory.EventCount < max_event_count && !finalreached)
		{
			auto& cat = trajectory.Category;
			if (cat.IsAwaitEvent())
			{
				ASSERT_NO_THROW(
					mdp->IncorporateEvent({ &trajectory,1 });
				);
			}
			else if (cat.IsAwaitAction())
			{
				ASSERT_NO_THROW(
					policy->SetAction({ &trajectory,1 });
				);
				ASSERT_NO_THROW(
					mdp->IncorporateAction({ &trajectory,1 });
				);
			}
			else if (cat.IsFinal())
			{
				finalreached = true;
			}
		}

	}



	TEST(lost_sales, default_mdp_config_0) {
		std::string model_name = "lost_sales";
		std::string config_name = "default_mdp_config_0.json";
		ExecuteTest(model_name, config_name);
	
	}
}