//! A small multi-threaded & data oriented water simulator using the pipes model

// A cell has these properties:
//	Terrain height
//	Water height
//	Sediment height
//	4 flux values, one for each direction

// Of the heights there are also temp versions, whenever something is calculated based on a neighbooring value of the same type we don't want to use the calculated result already

// To run a simulation step you'd need to:
//	1: Add rainfall (Water)
//	2: Update the flux values (Terrain, Water, Sediment, Flux)
//	3: Apply boundary condition to the flux field (Flux)
//	4a: Move the water & sediment given by the flux field (Water, Sediment, Flux)
//	4b: Limit the steepness of the terrain (Terrain)
//	5: Apply the calculated values (Terrain, Water, Sediment)
//	6a: Erode & deposit sediment to the terrain (Terrain, Water, Sediment, Flux)
//	6b: Evaporate left over water (Water)

use crossbeam::scope;
use crossbeam::deque::{Steal, Worker};

#[repr(C)]
pub struct SimulationVariables {
	pub thread_count : usize,
	pub rainfall : f32,
	pub evaporation : f32,
	pub dt : f32,
	pub gravity : f32,
	pub pipe_length : f32,
	pub sediment_capacity : f32,
	pub dissolve_constant : f32,
	pub deposition_constant : f32,
	pub max_step : f32,
	pub rain_random : i32,
}

impl SimulationVariables {
	pub fn new() -> Self
	{
		SimulationVariables {
			thread_count: num_cpus::get(),
			rainfall: 0.02,
			evaporation: 0.025,
			dt: 0.05,
			gravity: 9.81,
			pipe_length: 1.0,
			sediment_capacity: 0.15,
			dissolve_constant: 0.025,
			deposition_constant: 2.0,
			max_step: (60.0 * std::f32::consts::PI / 180.0).tan(),
			rain_random: 10,
		}
	}
}

#[derive(Clone, Debug)]
struct Pipe {
	flux : f32,
}

impl Pipe {
	fn new() -> Self {
		Pipe { flux: 0.0 }
	}

	fn update(&mut self, sv : &SimulationVariables, height: f32, height_out: f32) {
		let new = self.flux + sv.dt * sv.gravity * (height - height_out) / sv.pipe_length;
		self.flux = new.max(0.0);
	}

	fn scale_back(&mut self, k: f32)
	{
		self.flux *= k;
	}
}

#[derive(Clone, Debug)]
struct CellPipes {
	left : Pipe,
	right : Pipe,
	up : Pipe,
	down : Pipe,
}

impl CellPipes {
	fn new() -> Self {
		CellPipes {
			left: Pipe::new(),
			right: Pipe::new(),
			up: Pipe::new(),
			down: Pipe::new(),
		}
	}

	fn out_flux(&self) -> f32 {
		self.left.flux + self.right.flux + self.up.flux + self.down.flux
	}

	fn scale_back(&mut self, water_volume: f32)
	{
		let k = (water_volume / self.out_flux()).min(1.0);	// TODO: Check for infs or nans

		self.left.scale_back(k);
		self.right.scale_back(k);
		self.up.scale_back(k);
		self.down.scale_back(k);
	}
}

struct Grid {
	size_x: usize,
	size_y: usize,
}

struct ThreadedData<T> {
	write_into: T,
	y: usize,
}

impl Grid {
	fn new(size_x: usize, size_y: usize) -> Self {
		Grid {
			size_x, size_y
		}
	}

	fn left(&self, loc: usize) -> usize { loc - 1 }
	fn right(&self, loc: usize) -> usize { loc + 1 }
	fn up(&self, loc: usize) -> usize { loc - self.size_x }
	fn down(&self, loc: usize) -> usize { loc + self.size_x }


	fn apply(&self, func: &mut impl FnMut(usize) -> ()) {
		for y in 1..(self.size_y-1) {
			for x in 1..(self.size_x-1) {
				func(x + y * self.size_x);
			}
		}
	}
	fn apply_threaded<T>(&self, write_into: &mut [T], num_threads: usize, func: &(impl Fn(&mut T, usize) -> () + std::marker::Send + std::marker::Sync))
		where T : std::marker::Send + std::marker::Sync {
		let to_run = Worker::new_fifo();

		let mut leftover = write_into;
		for y in 1..(self.size_y-1) {
			let (base, new_leftover) = leftover.split_at_mut(self.size_x);
			leftover = new_leftover;

			to_run.push(ThreadedData {
				write_into: base,
				y,
			});
		}
		
		scope(|s| {
			let stealer = to_run.stealer();

			for _ in 0..num_threads {
				let stealer = stealer.clone();

				s.spawn(move |_| {
					loop {
						match stealer.steal() {
							Steal::Success(run) => {
								for x in 1..(self.size_x-1) {
									func(&mut run.write_into[x], x + run.y * self.size_x);
								}
							},
							Steal::Retry => continue,
							Steal::Empty => break,
						};
					}
				});
			}
		}).expect("Water simulation function paniced!");
	}
	// I do not like this copyied bit of code, its %90 the same, other than having 2 things to write into, but i have no clue if its possible to use generics in this case, you might make T a (&mut T1, &mut T2), and the argument too, but how do you do the leftover bit then? and the indexing later on?
	fn apply_threaded_double<T1, T2>(&self, write_into1: &mut [T1], write_into2: &mut [T2], num_threads: usize, func: &(impl Fn(&mut T1, &mut T2, usize) -> () + std::marker::Send + std::marker::Sync))
		where T1 : std::marker::Send + std::marker::Sync
			, T2 : std::marker::Send + std::marker::Sync {
		let to_run = Worker::new_fifo();

		let mut leftover1 = write_into1;
		let mut leftover2 = write_into2;
		for y in 1..(self.size_y-1) {
			let (base1, new_leftover1) = leftover1.split_at_mut(self.size_x);
			leftover1 = new_leftover1;
			let (base2, new_leftover2) = leftover2.split_at_mut(self.size_x);
			leftover2 = new_leftover2;

			to_run.push(ThreadedData {
				write_into: (base1, base2),
				y,
			});
		}
		
		scope(|s| {
			let stealer = to_run.stealer();

			for _ in 0..num_threads {
				let stealer = stealer.clone();

				s.spawn(move |_| {
					loop {
						match stealer.steal() {
							Steal::Success(run) => {
								for x in 1..(self.size_x-1) {
									func(&mut run.write_into.0[x], &mut run.write_into.1[x], x + run.y * self.size_x);
								}
							},
							Steal::Retry => continue,
							Steal::Empty => break,
						};
					}
				});
			}
		}).expect("Water simulation function paniced!");
	}
}

struct Heights {
	terrain_height: Box<[f32]>,
	water_height: Box<[f32]>,
	sediment_height: Box<[f32]>,
}

impl Heights {
	pub fn get_total_liquid_height(&self, loc: usize) -> f32 {
		self.water_height[loc] + self.sediment_height[loc]
	}
	pub fn get_total_height(&self, loc: usize) -> f32 {
		self.terrain_height[loc] + self.get_total_liquid_height(loc)
	}
	fn get_height_pr(&self, loc: usize, height: f32) -> f32 {
		return height / self.get_total_liquid_height(loc)
	}
	fn get_water_for_height(&self, loc: usize, height: f32) -> f32 {
		self.get_height_pr(loc, height) * self.water_height[loc]
	}
	fn get_sediment_for_height(&self, loc: usize, height: f32) -> f32 {
		self.get_height_pr(loc, height) * self.sediment_height[loc]
	}

	fn new(size: usize) -> Self {
		Self {
			terrain_height: vec![0.0; size].into_boxed_slice(),
			water_height: vec![0.0; size].into_boxed_slice(),
			sediment_height: vec![0.0; size].into_boxed_slice(),
		}
	}
}

pub struct WaterSimulator
{
	pub sv: SimulationVariables,

	grid: Grid,

	pipes: Box<[CellPipes]>,

	heights: Heights,
	temp_heights: Heights,
}

impl WaterSimulator {
	pub fn new(sv: SimulationVariables, size_x: usize, size_y: usize) -> Self {
		let num_elems = size_x * size_y;

		WaterSimulator {
			sv,
			grid: Grid::new(size_x, size_y),

			pipes: vec![CellPipes::new(); num_elems].into_boxed_slice(),

			heights: Heights::new(num_elems),
			temp_heights: Heights::new(num_elems),
		}
	}

	// Getters
	pub fn size_x(&self) -> usize { self.grid.size_x }
	pub fn size_y(&self) -> usize { self.grid.size_y }

	pub fn terrain_height(&self) -> &[f32] { &self.heights.terrain_height }
	pub fn water_height(&self) -> &[f32] { &self.heights.water_height }
	pub fn sediment_height(&self) -> &[f32] { &self.heights.sediment_height }

	pub fn terrain_height_mut(&mut self) -> &mut [f32] { &mut self.heights.terrain_height }
	pub fn water_height_mut(&mut self) -> &mut [f32] { &mut self.heights.water_height }
	pub fn sediment_height_mut(&mut self) -> &mut [f32] { &mut self.heights.sediment_height }
	
	// utility functions
	fn get_sediment_transport_capacity(sv: &SimulationVariables, grid: &Grid, pipes: &[CellPipes], loc: usize) -> f32 {
		let vel_x = ((pipes[grid.left(loc)].right.flux - pipes[loc].left.flux) + (pipes[loc].right.flux - pipes[grid.right(loc)].left.flux)) / 2.0;
		let vel_y = ((pipes[grid.down(loc)].up.flux - pipes[loc].down.flux) + (pipes[loc].up.flux - pipes[grid.up(loc)].down.flux)) / 2.0;

		//let velocity = (vel_x * vel_x + vel_y * vel_y).sqrt();
		let velocity = vel_x.abs() + vel_y.abs();

		sv.sediment_capacity * velocity	// Note: This does not take into account the local tilt angle, should be multiplied by sin(angle)
	}

	// Steps:
	fn add_rainfall(&mut self) {
		// TODO: Random rainfall
		if self.sv.thread_count == 0 {	// NOTE: This seems to be faster without threading
			self.grid.apply(&mut |loc| self.heights.water_height[loc] += self.sv.dt * self.sv.rainfall );
		} else {
			self.grid.apply_threaded(&mut self.heights.water_height, self.sv.thread_count, &|height, _| *height += self.sv.dt * self.sv.rainfall );
		}
	}
	fn update_flux(&mut self) {
		if self.sv.thread_count == 0 {
			self.grid.apply(&mut |loc| {
				let here_height = self.heights.get_total_height(loc);
	
				let cell_pipes = &mut self.pipes[loc];
				cell_pipes.left.update(&self.sv, here_height, self.heights.get_total_height(self.grid.left(loc)));
				cell_pipes.right.update(&self.sv, here_height, self.heights.get_total_height(self.grid.right(loc)));
				cell_pipes.up.update(&self.sv, here_height, self.heights.get_total_height(self.grid.up(loc)));
				cell_pipes.down.update(&self.sv, here_height, self.heights.get_total_height(self.grid.down(loc)));
	
				cell_pipes.scale_back(self.heights.get_total_liquid_height(loc));
			});
		} else {
			self.grid.apply_threaded(&mut self.pipes, num_cpus::get(), &|cell_pipes, loc| {
				let here_height = self.heights.get_total_height(loc);
	
				cell_pipes.left.update(&self.sv, here_height, self.heights.get_total_height(self.grid.left(loc)));
				cell_pipes.right.update(&self.sv, here_height, self.heights.get_total_height(self.grid.right(loc)));
				cell_pipes.up.update(&self.sv, here_height, self.heights.get_total_height(self.grid.up(loc)));
				cell_pipes.down.update(&self.sv, here_height, self.heights.get_total_height(self.grid.down(loc)));
	
				cell_pipes.scale_back(self.heights.get_total_liquid_height(loc));
			});
		}
	}
	fn apply_boundary_condition(&mut self) {
		// things cannot flow out
		for x in 1..self.grid.size_x-1 {
			self.pipes[x + self.grid.size_x].up.flux = 0.0;
			self.pipes[x + self.grid.size_x * (self.grid.size_y - 2)].down.flux = 0.0;
		}
		for y in 1..self.grid.size_y-1 {
			self.pipes[1 + self.grid.size_x * y].left.flux = 0.0;
			self.pipes[self.grid.size_x - 2 + self.grid.size_x * y].right.flux = 0.0;
		}
	}
	fn update_water_and_sediment(&mut self) {
		let volume_per_second_to_height_per_dt = self.sv.dt / self.sv.pipe_length / self.sv.pipe_length;
		
		// temp water = water - water_for_volume(flux out) + water_for_volume(flux in)
		// temp sediment = sediment - sediment_for_volume(flux out) + sediment_for_volume(flux in)

		if self.sv.thread_count == 0 {
			self.grid.apply(&mut |loc| {

				self.temp_heights.water_height[loc] = self.heights.water_height[loc] - self.heights.get_water_for_height(loc, self.pipes[loc].out_flux() * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.left(loc), self.pipes[self.grid.left(loc)].right.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.right(loc), self.pipes[self.grid.right(loc)].left.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.up(loc), self.pipes[self.grid.up(loc)].down.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.down(loc), self.pipes[self.grid.down(loc)].up.flux * volume_per_second_to_height_per_dt);

				self.temp_heights.sediment_height[loc] = self.heights.sediment_height[loc] - self.heights.get_sediment_for_height(loc, self.pipes[loc].out_flux() * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.left(loc), self.pipes[self.grid.left(loc)].right.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.right(loc), self.pipes[self.grid.right(loc)].left.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.up(loc), self.pipes[self.grid.up(loc)].down.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.down(loc), self.pipes[self.grid.down(loc)].up.flux * volume_per_second_to_height_per_dt);
			});
		} else {
			self.grid.apply_threaded_double(&mut self.temp_heights.water_height, &mut self.temp_heights.sediment_height, self.sv.thread_count, &|out_height, out_sediment, loc| {
				*out_height = self.heights.water_height[loc] - self.heights.get_water_for_height(loc, self.pipes[loc].out_flux() * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.left(loc), self.pipes[self.grid.left(loc)].right.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.right(loc), self.pipes[self.grid.right(loc)].left.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.up(loc), self.pipes[self.grid.up(loc)].down.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_water_for_height(self.grid.down(loc), self.pipes[self.grid.down(loc)].up.flux * volume_per_second_to_height_per_dt);

				*out_sediment = self.heights.sediment_height[loc] - self.heights.get_sediment_for_height(loc, self.pipes[loc].out_flux() * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.left(loc), self.pipes[self.grid.left(loc)].right.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.right(loc), self.pipes[self.grid.right(loc)].left.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.up(loc), self.pipes[self.grid.up(loc)].down.flux * volume_per_second_to_height_per_dt)
					+ self.heights.get_sediment_for_height(self.grid.down(loc), self.pipes[self.grid.down(loc)].up.flux * volume_per_second_to_height_per_dt);
			});
		}
	}
	fn update_steepness(&mut self) {
		// TODO: Actually limit steepness
		if self.sv.thread_count == 0 {
			self.grid.apply(&mut |loc| {
				self.temp_heights.terrain_height[loc] = self.heights.terrain_height[loc];
			});
		} else {
			self.grid.apply_threaded(&mut self.temp_heights.terrain_height, self.sv.thread_count, &|out_height, loc| {
				*out_height = self.heights.terrain_height[loc];	
			});
		}
	}
	fn apply_calculated_values(&mut self) {
		// Basically set x_height to temp_x_height
		// Does not matter what temp_x_height gets set to since that gets overridden anyway

		// Considering they are all boxed, i can just swap the structs, very fast!
		std::mem::swap(&mut self.heights, &mut self.temp_heights);
	}
	fn update_erosion_and_deposition(&mut self) {
		if self.sv.thread_count == 0 {
			self.grid.apply(&mut |loc| {
				let stc = WaterSimulator::get_sediment_transport_capacity(&self.sv, &self.grid, &self.pipes, loc);
		
				let desired_change = stc - self.heights.sediment_height[loc];

				let speed = if desired_change > 0.0 { self.sv.dissolve_constant } else { self.sv.deposition_constant };

				let sediment_change = speed * self.sv.dt * desired_change;
				self.heights.terrain_height[loc] -= sediment_change;
				self.heights.sediment_height[loc] += sediment_change;
			});
		} else {
			self.grid.apply_threaded_double(&mut self.heights.terrain_height, &mut self.heights.sediment_height, self.sv.thread_count, &|terrain_out, sediment_out, loc| {
				let stc = WaterSimulator::get_sediment_transport_capacity(&self.sv, &self.grid, &self.pipes, loc);
		
				let desired_change = stc - *sediment_out;

				let speed = if desired_change > 0.0 { self.sv.dissolve_constant } else { self.sv.deposition_constant };

				let sediment_change = speed * self.sv.dt * desired_change;
				*terrain_out -= sediment_change;
				*sediment_out += sediment_change;
			});
		}
	}
	fn update_evaporation(&mut self) {
		self.grid.apply(&mut |loc| {
			self.heights.water_height[loc] *= 1.0 - (self.sv.evaporation * self.sv.dt);
		});
	}

	// And them all together:
	pub fn step(&mut self)
	{
		self.add_rainfall();
		self.update_flux();
		self.apply_boundary_condition();
		self.update_water_and_sediment();
		self.update_steepness();
		self.apply_calculated_values();
		self.update_erosion_and_deposition();
		self.update_evaporation();
	}
}
