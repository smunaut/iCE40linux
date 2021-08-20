#!/usr/bin/env python3

import sys

def main(argv0, *specs):
	pos = 0
	for spec in specs:
		p, fn = spec.split(':')
		p = int(p, 0)

		while pos < p:
			print("00000000")
			pos += 1

		with open(fn, 'r') as fh:
			for l in fh.readlines():
				print(l.strip())
				pos += 1

if __name__ == '__main__':
	sys.exit(main(*sys.argv))
