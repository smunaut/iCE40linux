#include <iostream>
#include <fstream>

#include <backends/cxxrtl/cxxrtl_vcd.h>

#include "top.h"

using namespace std;

int main(int argc, char *argv[])
{
	cxxrtl_design::p_top top;

	/* debug_items setup */
	cxxrtl::debug_items all_debug_items;
	top.debug_info(all_debug_items);

	/* vcd_writer setup */
	cxxrtl::vcd_writer vcd;
	vcd.timescale(10, "ns");
	vcd.add_without_memories(all_debug_items);

	std::ofstream waves(argc > 1 ? argv[1] : "waves.vcd");

	/* First sample */
	top.step();
	vcd.sample(0);

	/* Loop */
	for(int steps=0; steps<5000000; steps++) {
		/* Falling edge */
		top.p_clk__in.set<bool>(false);
		top.step();
		vcd.sample(steps*4 + 0);

		/* Rising edge */
		top.p_clk__in.set<bool>(true);
		top.step();
		vcd.sample(steps*4 + 2);

		/* Save */
		waves << vcd.buffer;
		vcd.buffer.clear();
	}
}
