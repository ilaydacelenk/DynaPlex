#include <iostream>
#include "dynaplex/dynaplexprovider.h"

using namespace DynaPlex;
int main() {

	auto& dp = DynaPlexProvider::Get();

	DynaPlex::VarGroup config;
	//retrieve MDP registered under the id string "driver_assignment":
	config.Add("id", "driver_assignment");
	config.Add("number_drivers", 5);
	config.Add("mean_order_per_time_unit", 5.0);
	config.Add("max_x", 55.0);
	config.Add("max_y", 6.0);
	config.Add("max_working_hours", 780.0);
	config.Add("horizon_length", 1440.0);
	config.Add("cost_per_km", 15.0);
	config.Add("penalty_for_workhour_violation", 50.0);

	//auto path_to_json = dp.FilePath({ "mdp_config_0", "driver_assignment" }, "mdp_config_0.json");
	//std::cout << "nesli2" << std::endl;

	////also possible to use a standard configuration file:
	//config = VarGroup::LoadFromFile(path_to_json);
	//std::cout << "nesli3" << std::endl;

	DynaPlex::MDP mdp = dp.GetMDP(config);

	auto policy = mdp->GetPolicy("greedy_policy");

	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",15},
		{"mini_batch_size", 64},
		{"max_training_epochs", 100},
		{"train_based_on_probs", false}
	};

	DynaPlex::VarGroup nn_architecture{
		{"type","mlp"},//mlp - multi-layer-perceptron. 
		{"hidden_layers",DynaPlex::VarGroup::Int64Vec{128, 64}}//Note: Input/output layer sizes are determined by MDP. 
	};
	int64_t num_gens = 1;

	DynaPlex::VarGroup dcl_config{
		//use defaults everywhere. 
		{"N",5000},//number of samples
		{"num_gens",num_gens},//number of neural network generations. default 5000
		{"M",300},//rollouts per action, default is 1000. 
		{"H",40 },//horizon, i.e. number of steps for each rollout.
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		{"enable_sequential_halving",true},
		{"retrain_lastgen_only",false},
		{"resume_gen",0}
	};

	try
	{

		auto dem = dp.GetDemonstrator();
		auto trace = dem.GetTrace(mdp);
		for (auto& thing : trace)
		{
		    std::cout << thing.Dump() << std::endl;
		}
		


		
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
		auto path = dp.System().filepath("dcl_lost_sales_example", "lost_sales_dcl_gen" + num_gens);
		//this is IOLocation/dcl_lost_sales_example/lost_sales_policy
		//IOLocation is typically specified in CMakeUserPresets.txt
		//saves two files, one .json file with the architecture (e.g. trained_lost_sales_policy.json), and another file with neural network weights (.pth):		
		dp.SavePolicy(last_policy, path);

		//This loads the policy again from the same path, automatically adding the right extensions:
		auto policy = dp.LoadPolicy(mdp, path);


		config.Set("p", 90.0);
		config.Set("h", 10.0);
		//for illustration purposes, create a different mdp 
		//that is compatible with the first - same number of features, same number of valid actions:
		DynaPlex::MDP different_mdp = dp.GetMDP(config);

		//It is possible to load the policy trained for one MDP, and make it applicable to another mdp:
		//this however only works if the two policies have consistent input and output dimensions, i.e.
		//same number of valid actions and same number of features. 
		auto different_policy = dp.LoadPolicy(different_mdp, path);
	}
	catch (const std::exception& e)
	{
		std::cout << "exception: " << e.what() << std::endl;
	}
	return 0;
}
