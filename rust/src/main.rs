use water_test::*;
use std::time::Instant;

fn main() {
	let mut sv = SimulationVariables::new();
	//sv.thread_count = 0;
	// diminishing returns are pretty harsh
	// "0" thread: 4s (no thread spawning)
	// 1 thread: 3.6s (why is this faster? now there is overhead of spawning a single thread and immidiately trying to join it)
	// 2 threads: 2.2s	(expected: 2s)
	// 3 threads: 1.7s	(expected: 1.3s)
	// 4 threads: 1.66s (expected: 1s)
	
	println!("Running with {} threads!", sv.thread_count);

	let mut ws = WaterSimulator::new(sv, 1024, 1024);
	
	let now = Instant::now();
	for i in 0..100 {
		ws.step();
		println!("Step {} completed!", i);
	}

	println!("It took {} ms!", now.elapsed().as_millis());
}
