#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/retrievestate.h"

using namespace DynaPlex;
int main() {

	//initialize
	auto& dp = DynaPlexProvider::Get();
	std::string model_name = "driver_assignment";
	std::string mdp_config_name = "mdp_config_0.json";
	//Get path to IO_DynaPlex/mdp_config_examples/airplane_mdp/mdp_config_name:
	std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
	auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
	auto mdp = dp.GetMDP(mdp_vars_from_json);

	std::cout << mdp_vars_from_json.Dump(1) << std::endl;

	
	auto policy = mdp->GetPolicy("greedy_policy"); //greedy_policy

	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",15},
		{"mini_batch_size", 64},
		{"max_training_epochs", 100},
		{"train_based_on_probs", false}
	};

	DynaPlex::VarGroup nn_architecture{
		{"type","mlp"},//mlp - multi-layer-perceptron. 
		{"hidden_layers",DynaPlex::VarGroup::Int64Vec{256, 128}}//Note: Input/output layer sizes are determined by MDP. 
	};
	int64_t num_gens = 1;

	DynaPlex::VarGroup dcl_config{
		//use defaults everywhere. 
		{"N",50000},//number of samples
		{"num_gens",num_gens},//number of neural network generations. default 5000
		{"M",300},//rollouts per action, default is 1000. 
		{"H",60 },//horizon, i.e. number of steps for each rollout.
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		//{"enable_sequential_halving",true},
		{"retrain_lastgen_only",false}
		//{"resume_gen",0}
	};

	try
	{

		//Create a trainer for the mdp, with appropriate configuratoin. 
		auto dcl = dp.GetDCL(mdp, policy, dcl_config);
		//this trains the policy, and saves it to disk.
		dcl.TrainPolicy();
		//using a dcl instance that has same parameterization (i.e. same config, same mdp), we may recover the trained polciies.

		//This gets the policy that was trained last:
		//auto policy = dcl.GetPolicy();
		//This gets policy with specific index:
		//auto first = dcl.GetPolicy(1);
		//This gets all trained policy, as well as the initial policy, in a vector:
		auto policies = dcl.GetPolicies();

		//Compare the various trained policies:
		auto comparer = dp.GetPolicyComparer(mdp);
		auto comparison = comparer.Compare(policies);
		for (auto& VarGroup : comparison)
		{
			std::cout << VarGroup.Dump() << std::endl;
		}

		//policies are automatically saved when training, but it may be usefull to save at custom location. 
		//To do so, we retrieve the policy, get a path where to save it, and thens ave it there/ 
		auto last_policy = dcl.GetPolicy();
		//gets a file_path without file extension (file extensions are automatically added when saving): 
		auto path = dp.System().filepath("dcl", "driver_assignment_mdp", "driver_assignment_policy_gen" + std::to_string(num_gens));
		//this is IOLocation/dcl_lost_sales_example/lost_sales_policy
		//IOLocation is typically specified in CMakeUserPresets.txt
		//saves two files, one .json file with the architecture (e.g. trained_lost_sales_policy.json), and another file with neural network weights (.pth):		
		dp.SavePolicy(last_policy, path);

		for (auto& VarGroup : comparer.Compare(policy, mdp->GetPolicy("random")))
		{
			std::cout << VarGroup.Dump() << std::endl;
		}

	}
	catch (const std::exception& e)
	{
		std::cout << "exception: " << e.what() << std::endl;
	}
	return 0;
}
