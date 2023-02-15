# Find PLL
pll = [v for k,v in ctx.cells if 'PLL' in v.type][0]

BASE = {
	('0000', '1001111', '100'): 15,
	('0000', '0110100', '011'): 20,
}[(pll.params['DIVR'], pll.params['DIVF'], pll.params['DIVQ'])]

ctx.addClock("clk_1x", 1 * BASE)
ctx.addClock("clk_4x", 4 * BASE)
ctx.addClock("clk_rd", 4 * BASE)
