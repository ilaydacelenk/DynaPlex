#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
#include "dynaplex/dynaplexprovider.h"
namespace DynaPlex::Tests {	
	TEST(reel_allocation2, mdp_config_2) {
		std::string model_name = "reel_allocation2"; // this should match the id and namespace name discussed earlier
		std::string config_name = "mdp_config_2.json";

		Tester tester{};
		tester.ExecuteTest(model_name, config_name);
	}
	TEST(reel_allocation2, mdp_config_5) {
		std::string model_name = "reel_allocation2"; // this should match the id and namespace name discussed earlier
		std::string config_name = "mdp_config_5.json";

		Tester tester{};
		tester.ExecuteTest(model_name, config_name);
	}
	TEST(reel_allocation2, mdp_config_2_policy) {
		std::string model_name = "reel_allocation2"; // this should match the id and namespace name discussed earlier
		std::string config_name = "mdp_config_2.json";
		std::string policy_config_name = "policy_config_0.json";

		Tester tester{};
		tester.ExecuteTest(model_name, config_name, policy_config_name);
	}
}