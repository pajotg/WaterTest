use water_test::*;

#[no_mangle]
pub extern "C" fn print_stuff_in_rust()
{
	println!("I am printing things in rust!");
}


// Functions to export:
// New simulation variables V
// New simulation
// Step simulation
// Getters for simulation heights & size

#[no_mangle]
pub extern "C" fn new_simulation_variables() -> SimulationVariables { SimulationVariables::new() }

#[no_mangle]
pub extern "C" fn new_simulation(sv : SimulationVariables, size_x : usize, size_y : usize) -> Box<WaterSimulator> {
	Box::new(WaterSimulator::new(sv, size_x, size_y))
}

#[no_mangle]
pub extern "C" fn step_simulation(sim: &mut WaterSimulator) {
	sim.step();
}


#[no_mangle]
pub extern "C" fn get_terrain_height(sim: &mut WaterSimulator, x: usize, y: usize) -> f32 { sim.terrain_height()[x + sim.size_x() * y] }
#[no_mangle]
pub extern "C" fn get_water_height(sim: &mut WaterSimulator, x: usize, y: usize) -> f32 { sim.water_height()[x + sim.size_x() * y] }
#[no_mangle]
pub extern "C" fn get_sediment_height(sim: &mut WaterSimulator, x: usize, y: usize) -> f32 { sim.sediment_height()[x + sim.size_x() * y] }