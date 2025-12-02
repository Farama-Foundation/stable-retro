#!/usr/bin/env python3

from time import sleep
from sys import argv
from os import environ
import subprocess

def get_next_line(p):
	line = ""

	while line[0:5] != "CYCLE":
		line = p.readline().decode()

		if (len(line) == 0):
			sleep(0.001)
		elif line[0:5] != "CYCLE":
			print(line[:-1])

	return line

def print_differences(inter, dynarec):
	inter_array = inter.split(" ")
	inter_dict = dict(zip(inter_array[::2], inter_array[1::2]))
	dynarec_array = dynarec.split(" ")
	dynarec_dict = dict(zip(dynarec_array[::2], dynarec_array[1::2]))

	diff = dict([(k, (inter_dict[k], dynarec_dict[k])) for k in inter_dict.keys() if inter_dict[k] != dynarec_dict[k]])

	print("\nDifferences:")
	print("{:15}{:15}{:15}".format("", "Interpreter", "Dynarec"))
	for k in diff:
		print("{:15}{:15}{:15}".format(k, diff[k][0], diff[k][1]))

def print_mismatch(inter, dynarec, oldline):
	print("\nMismatch!")
	print(inter + " - Interpreter")
	print(dynarec + " - Dynarec")
	print("State before the mismatch:")
	print(oldline)
	print_differences(inter, dynarec)

def read_loop(p1, p2):
	oldline = ""

	while True:
		line1 = get_next_line(p1)
		line2 = get_next_line(p2)

		if line1 != line2:
			# TODO: Proper matching

			# Lightrec might be lagging behind
			#if line1[0:16] != line2[0:16]:
			if line1[6:16] != line2[6:16]:
				cycle1 = int(line1[6:16], 16)
				cycle2 = int(line2[6:16], 16)

				if cycle1 < cycle2:
					print(line2[:-1] + " - Dynarec")

					while cycle1 < cycle2:
						print(line1[:-1] + " - Interpreter lagging behind")
						print_differences(line1[:-1], line2[:-1])
						line1 = get_next_line(p1)
						cycle1 = int(line1[6:16], 16)

					while cycle1 > cycle2:
						print(line2[:-1] + " - Dynarec lagging behind")
						print_differences(line1[:-1], line2[:-1])
						line2 = get_next_line(p2)
						cycle2 = int(line2[6:16], 16)

					if line1 != line2:
						print_mismatch(line1[:-1], line2[:-1], oldline)
						break

				if cycle2 < cycle1:
					print(line1[:-1] + " - Interpreter")

					while cycle1 > cycle2:
						print(line2[:-1] + " - Dynarec lagging behind")
						print_differences(line1[:-1], line2[:-1])
						line2 = get_next_line(p2)
						cycle2 = int(line2[6:16], 16)

					while cycle1 < cycle2:
						print(line1[:-1] + " - Interpreter lagging behind")
						print_differences(line1[:-1], line2[:-1])
						line1 = get_next_line(p1)
						cycle1 = int(line1[6:16], 16)

					if line1 != line2:
						print_mismatch(line1[:-1], line2[:-1], oldline)
						break

				if line1 == line2:
					oldline = line1[:-1]
					print(oldline[:16] + " - Match")
				continue

			print_mismatch(line1[:-1], line2[:-1], oldline)
			break
		else:
			oldline = line1[:-1]

def main():
	with subprocess.Popen(['./pcsx'] + argv[1:], env={ **environ, 'LIGHTREC_DEBUG': '1', 'LIGHTREC_INTERPRETER': '1' }, stdout=subprocess.PIPE, bufsize=1) as fifo_int:
		with subprocess.Popen(['./pcsx'] + argv[1:], env={ **environ, 'LIGHTREC_DEBUG': '1' }, stdout=subprocess.PIPE, bufsize=1) as fifo_jit:
			read_loop(fifo_int.stdout, fifo_jit.stdout)

if __name__ == '__main__':
	main()
