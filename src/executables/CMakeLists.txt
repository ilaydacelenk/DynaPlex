
cmake_minimum_required (VERSION 3.21)

if(MPI_FOUND)
add_subdirectory(mpi_tests)
endif()
add_subdirectory(memory_checks)
add_subdirectory(executable_example)
add_subdirectory(executable_order_picking)
add_subdirectory(parametric_policy_opt)
add_subdirectory(dcl_example)
add_subdirectory(lostsales_paper_results)
add_subdirectory(binpacking_evaluate)
add_subdirectory(perishables_paper_results)
add_subdirectory(ilayda_executable)
add_subdirectory(airplane_mdp) # Added
add_subdirectory(reel_allocation_mdp) # Added
add_subdirectory(reel_allocation_mdp2) # Added
add_subdirectory(reel_allocation_mdp2_arrival) # Added


