#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
#include "dynaplex/dynaplexprovider.h"
namespace DynaPlex::Tests {	
	TEST(two_slot, mdp_config_0) {
		std::string model_name = "two_slot"; // this should match the id and namespace name discussed earlier
		std::string config_name = "mdp_config_0.json";
		Tester tester{};
		tester.ExecuteTest(model_name, config_name);
	}

}