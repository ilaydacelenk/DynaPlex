#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
#include "dynaplex/dynaplexprovider.h"
namespace DynaPlex::Tests {	
	TEST(driver_assignment, mdp_config_0) {
		std::string model_name = "driver_assignment"; // this should match the id and namespace name discussed earlier
		std::string config_name = "mdp_config_0.json";

		Tester tester{};
		tester.ExecuteTest(model_name, config_name);
	}
	
	TEST(driver_assignment, mdp_config_0_policy) {
		std::string model_name = "driver_assignment"; // this should match the id and namespace name discussed earlier
		std::string config_name = "mdp_config_0.json";
		std::string policy_config_name = "policy_config_0.json";

		Tester tester{};
		tester.ExecuteTest(model_name, config_name, policy_config_name);
	}
}