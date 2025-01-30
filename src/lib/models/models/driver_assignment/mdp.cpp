#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"


namespace DynaPlex::Models {
	namespace driver_assignment /*keep this in line with id below and with namespace name in header*/
	{
		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			//this typically works, but state.cat must be kept up-to-date when modifying states. 
			return state.cat;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("current_driver_list_conc", current_driver_list_conc);
			vars.Add("current_order_typelist", current_order_typelist);
			//add any other state variables. 
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("current_driver_list_conc", state.current_driver_list_conc);
			vars.Get("current_order_typelist", state.current_order_typelist);
			//initiate any other state variables. 
			return state;
		}

		MDP::State MDP::GetInitialState(DynaPlex::RNG& rng) const
		{
			//driver starting locations, so x and y are randomly decided for the initial state. 
			//the time starts at 0 and the remaining work hours of the drivers are set to max working hours. 
			//the next time beign available for each driver is 0 since there isnt any task on thme yet. 
			// 
			//driver icin state vectorum, x ve y koordinatlari sonra next time being available sonra da rem working hours o yuzden 
			// vectordeki pozisyonlair belirlemek zorudnaydim. 
			State state{};
			state.cat = StateCategory::AwaitEvent();//or AwaitAction(), depending on logic
			state.current_time = 0.0;
			state.current_driver_list_conc.reserve(number_drivers * 5);
			for (int64_t i = 0; i < number_drivers; i++) {
				state.current_driver_list_conc.push_back(0.0);
				double current_x = rng.genUniform() * max_x + min_x;
				double current_y = rng.genUniform() * max_y + min_y;
				state.current_driver_list_conc.push_back(current_x);
				state.current_driver_list_conc.push_back(current_y);
				state.current_driver_list_conc.push_back(state.current_time);
				state.current_driver_list_conc.push_back(max_working_hours);
			}

			//order state vectorun icinde pick x ve y deliver x ve y var ama biz bunlara bir sey tanimlamadik 
			// cunku zaten random event ile gelecekler
			state.current_order_typelist.resize(4, 0.0);

			return state;
		}

		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			//Needs to update later:
			vars.Add("valid_actions", number_drivers);

			//discount factor default ataniyor eger 1 ise, ve suan infinite horizon bunun sebebi eger finite secersem decision
			//epochlarim discrete time periodlar olmak zorundaymis simdi sadece horizon length define ediyorum ve isimi cozuyor.

			return vars;
		}

		MDP::MDP(const VarGroup& config)
		{
			config.Get("number_drivers", number_drivers);
			config.Get("mean_order_per_time_unit", mean_order_per_time_unit);
			config.Get("max_x", max_x);
			config.Get("max_y", max_y);
			config.Get("max_working_hours", max_working_hours);
			config.Get("horizon_length", horizon_length);
			config.Get("cost_per_km", cost_per_km);
			config.Get("penalty_for_workhour_violation", penalty_for_workhour_violation);


			//In principle, state variables should be initiated as follows:
			//config.Get("name_of_variable",name_of_variable); 
		}

		MDP::Event MDP::GetEvent(RNG& rng) const {
			//burada event gelisleri arasi sure exponential dagiliyor bunu bir sekilde uniform dist modift ederek cozduk. 
			double random_variable_for_interarrivaltime = rng.genUniform();
			double interarrivaltime = std::log(1 - random_variable_for_interarrivaltime) / (-mean_order_per_time_unit);
			double pickup_x = rng.genUniform() * max_x + min_x;
			double pickup_y = rng.genUniform() * max_y + min_y;
			double deliver_x = rng.genUniform() * max_x + min_x;
			double deliver_y = rng.genUniform() * max_y + min_y;
			//burada randomly create ettigim itemlardan bir event vectoru yaratiyorum. 
			std::vector<double> event_vec{ interarrivaltime, pickup_x, pickup_y, deliver_x, deliver_y };

			return event_vec;
		}

		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			double interarrivaltime = event[0];//bu aslinda vector olarak belirttigimiz icin gereksiz ama clear olsun diye her event item bunlari define ettik. 
			state.current_order_typelist[0] = event[1];
			state.current_order_typelist[1] = event[2];
			state.current_order_typelist[2] = event[3];
			state.current_order_typelist[3] = event[4];
			state.current_time += interarrivaltime;

			for (int64_t i = 0; i < number_drivers; i++) {//burada her driver icin 5 info tutuyoruz.i*5 dedigimiz her driver icin segment
				//bu durum eger driver busy ise ve i*5+3 dedigimiz de driverin 4. itemi yani eger next time available olma saati kucukse driver available degil demek.
				//bu durumda yeni bir event ile driver tekrar available olabilir ve bu da next availability time i suan yapar. 
				if (state.current_driver_list_conc[i * 5] == 1.0 && state.current_time > state.current_driver_list_conc[i * 5 + 3]) {
					state.current_driver_list_conc[i * 5] = 0.0;
					state.current_driver_list_conc[i * 5 + 3] = state.current_time;
				}
				else {
					//eger driver zaten bostaysa, yeni event gelmesi durumunda busy/idle state degismez, next avail. time suan olarak devam eder. 
					state.current_driver_list_conc[i * 5 + 3] = state.current_time;
				}
			}
			state.cat = StateCategory::AwaitAction();// bunu getirmezsek eventi triggerlamiyor, bu sart. 

			return 0.0;
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
			double cost = 0.0;
			//ilk olarak driverlarin pick lokasyonlarina mesafesi hesaplaniyor. bu bulunduklari x y koordinatlari.
			double pick_up_distance_x = std::abs(state.current_driver_list_conc[action * 5 + 1] - state.current_order_typelist[0]);
			double pick_up_distance_y = std::abs(state.current_driver_list_conc[action * 5 + 2] - state.current_order_typelist[1]);
			//simdi de pick lokasyonlarindan deliverlara olan uzaklik hesaplaniyor. 
			double delivery_distance_x = std::abs(state.current_order_typelist[0] - state.current_order_typelist[2]);
			double delivery_distance_y = std::abs(state.current_order_typelist[1] - state.current_order_typelist[3]);
			double total_dist = pick_up_distance_x + pick_up_distance_y + delivery_distance_x + delivery_distance_y;
			// calculate distance
			cost += cost_per_km * total_dist;
			//costu her bu ikili icin hesaplamis oluyorum. 
			if (state.current_driver_list_conc[action * 5] == 0.0) {
				//if the driver is available he becomes busy with the action and his next availability time updates. 
				//rem work houru burada update etmeme gerek yok cunku bu if statementa bagli olmaksizin guncellencek. 
				//current time farkli iki duruma gore degisiyor ondan boyle yaptik. 
				state.current_driver_list_conc[action * 5] = 1.0; // driver is busy now
				// driver's next availability
				state.current_driver_list_conc[action * 5 + 3] = state.current_time + total_dist;
			}
			else {

				state.current_driver_list_conc[action * 5 + 3] += total_dist;
			}
			// driver's new location after delivery
			state.current_driver_list_conc[action * 5 + 1] = state.current_order_typelist[2];//orderin deliver lokasyonu x
			state.current_driver_list_conc[action * 5 + 2] = state.current_order_typelist[3];//orderin deliver lokasyonu y
			state.current_driver_list_conc[action * 5 + 4] -= total_dist;//rem work hours oncekinden gecen sure cikarilarak bulunuyor. 
			if (state.current_driver_list_conc[action * 5 + 4] < 0.0) {//eger rem work hours 0in altina duserse penalty cost ekliyoruz. 
				cost += penalty_for_workhour_violation * (-state.current_driver_list_conc[action * 5 + 4]); //overtime cost
			}

			//implement change to state. 
			state.cat = StateCategory::AwaitEvent();
			//ne zaman duruyoruz karari da burada veriliyor. 
			if (state.current_time >= horizon_length) {
				state.current_time = 0.0;
				for (int64_t i = 0; i < number_drivers; i++) {
					state.current_driver_list_conc[i * 5] = 0.0;
					state.current_driver_list_conc[i * 5 + 3] = 0.0;
					state.current_driver_list_conc[i * 5 + 4] = max_working_hours;
				}
			}

			return cost;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			// tanimladigimiz action vectoru icerisinde belirli indexli actionlara aslinda remaning work 
			// hour kontrolu uzerinden error vermesi icin imkan sagliyoruz ki atama yapmasin
			// driver busy-not possible to assign
			return true;
		}

		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			

			features.Add(state.current_driver_list_conc); 
			features.Add(state.current_order_typelist);
			features.Add(state.current_time);
		}

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{//Here, we register any custom heuristics we want to provide for this MDP.	
		 //On the generic DynaPlex::MDP constructed from this, these heuristics can be obtained
		 //in generic form using mdp->GetPolicy(VarGroup vars), with the id in var set
		 //to the corresponding id given below.
			registry.Register<GreedyPolicy>("greedy_policy",
				//greedy policy, en yakin driver assign edilir ordera
				"This policy will take the closest driver to the order when asked for an action. ");
		}

		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("driver_assignment", "nonempty description", registry);
			//To use this MDP with dynaplex, register it like so, setting name equal to namespace and directory name
			// and adding appropriate description. 
			//DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
			//	"<id of mdp goes here, and should match namespace name and directory name>",
			//	"<description goes here>",
			//	registry); 
		}
	}
}

