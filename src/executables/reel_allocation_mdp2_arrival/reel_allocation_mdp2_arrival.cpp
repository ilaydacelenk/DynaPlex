#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/retrievestate.h"
#include "dynaplex/models/reel_allocation2_arrival/mdp.h"


using namespace DynaPlex;

int main() {

    //initialize
    auto& dp = DynaPlexProvider::Get();
    std::string model_name = "reel_allocation2_arrival";
    std::string mdp_config_name = "mdp_config_2.json";
    //Get path to IO_DynaPlex/mdp_config_examples/airplane_mdp/mdp_config_name:
    std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
    auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
    auto mdp = dp.GetMDP(mdp_vars_from_json);

    std::cout << mdp_vars_from_json.Dump(1) << std::endl;
    //for illustration purposes, create a different mdp
    //that is compatible with the first - same number of features, same number of valid actions:
    //DynaPlex::MDP different_mdp = dp.GetMDP(mdp_vars_from_json);

    //auto policy = mdp->GetPolicy("index_policy_discarded3");
    auto policy = mdp->GetPolicy("random");

    DynaPlex::VarGroup nn_training{
        {"early_stopping_patience",15},
        {"mini_batch_size", 64},
        {"max_training_epochs", 100},
        {"train_based_on_probs", false}
    };

    DynaPlex::VarGroup nn_architecture{
        {"type","mlp"},//mlp - multi-layer-perceptron. 
        {"hidden_layers",DynaPlex::VarGroup::Int64Vec{64, 64}} // one hidden layer - 128, 64 - make 512,256, 128
        //Note: Input/output layer sizes are determined by MDP. 
    };
    int64_t num_gens = 1;

    DynaPlex::VarGroup dcl_config{
        {"N",5000}, //number of samples, default 5000 - make 50000
        {"num_gens",num_gens}, //number of neural network generations.
        {"M",300}, //rollouts per action, default 1000. - make 300 - 1000
        {"nn_architecture",nn_architecture},
        {"nn_training",nn_training},
        {"retrain_lastgen_only",false},
        {"H",25}, //depth of the rollouts (horizon length) default 40 - make 40 - 25
        {"L",100} //lenght of the warm-up period, default 100 - make 300
    };

    try
    {
        // added by willem 
        auto dem = dp.GetDemonstrator();
        auto trace = dem.GetTrace(mdp);
        for (auto& thing : trace)
        {
            std::cout << thing.Dump() << std::endl;
        }
        //return 0;

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
        //auto comparison = comparer.Compare(policy, mdp->GetPolicy("random"));

        for (auto& VarGroup : comparison)
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
