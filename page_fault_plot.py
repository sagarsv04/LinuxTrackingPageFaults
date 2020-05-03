#!/bin/python
import os
import sys
import numpy as np
from matplotlib import pyplot as plt


def plot_page_fault(address_array, time_array, process_id):
	# time on x axis
	fig1 = plt.figure(1)
	ax1 = fig1.gca()
	ax1.scatter(time_array, address_array, s=25)
	ax1.set_aspect('equal')
	plt.title("Page Fault Plot For Process {0}".format(process_id))
	plt.xlabel("Time in nsec")
	plt.ylabel("Virtual Address")
	plt.tight_layout()
	plt.show()
	return 0


def find_nearest_idx(array, value):
	array = np.asarray(array)
	idx = (np.abs(array - value)).argmin()
	return idx



def print_page_fault(address_array, time_array, process_id):

	char_array = np.array([[" " for _ in range(72+71)] for _ in range(32)])
	addr_max = address_array.max()
	time_max = time_array.max()
	print("{0} Plot for Process Id {1} {2}".format("#"*22, process_id, "#"*22))
	print("\n")

	addr_list = [0]*32
	time_list = [0]*(72+71)

	for row in range(0,32):
		# row = 0
		addr_list[row] = int(addr_max/(row+1))

	for col in range(0,72+71):
		# col = 0
		time_list[col]  = int(time_max/(col+1))

	for idx in range(address_array.shape[0]):
		# idx = 0
		addr = address_array[idx]
		time = time_array[idx]
		addr_idx = find_nearest_idx(addr_list, addr)
		time_idx = find_nearest_idx(time_list, time)
		char_array[addr_idx][time_idx] = "*"
	for idx in range(32):
		# idx = 0
		print("{0:10d} | {1}".format(addr_list[idx], "".join(char_array[idx])))
	print("{0:10} {1}".format(" ", "".join(["-"]*(72+71))))
	time_char_array = np.array([[" " for _ in range(72+71)] for _ in range(len(str(addr_max))+1)])

	for jdx in range(time_char_array.shape[0]):
		# jdx = 0
		for idx in range(72+71):
		# idx = 0
			if (idx % 2) == 0:
				time_char_array[jdx][idx] = "|"
			else:
				if jdx >= len(list(str(time_list[idx]))):
					pass
				else:
					time_char_array[jdx][idx] = list(str(time_list[idx]))[jdx]

	for idx in range(time_char_array.shape[0]):
		print("{0:10} {1}".format(" ", "".join(time_char_array[idx])))
	return 0


def process_file(file_path):
	# file_path = "./pf_probe_A.log"
	lines = None
	if os.path.exists(file_path):
		with open(file_path) as fd:
			lines = fd.readlines()

		address_list = []
		time_list = []
		process_id = 0
		# process file
		for line in lines:
			# line = lines[1]
			line_split = line.split(" ")
			if (("Address" in line_split)and("Time" in line_split)):
				time_list.append(int(line_split[-1].strip("\n")))
				address_list.append(int(line_split[-4], 0))
				if (process_id == 0):
					process_id = int(line_split[-9])
		address_array = np.array(address_list)
		time_array = np.array(time_list)
		plot_page_fault(address_array, time_array, process_id)
		print_page_fault(address_array, time_array, process_id)
	else:
		print("File {0} doesn't exists ...".format(file_path))
	return 0


def main():
	if len(sys.argv) != 2:
		print("Usage: python {0} <path of user read log file>".format(sys.argv[0]))
		return -1
	else:
		file_path = sys.argv[1]
		print("My Pid :: {0}".format(os.getpid()))
		process_file(file_path)
	return 0

if __name__ == "__main__":
	main()

71 seperators

7|3|2|1|1|1|1|9|8|7|7|6|6|5|5|4|4|4|4|3|3|3|3|3|3|3|2|2|2|2|2|2|2|2|2|2|2|2|2|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1|1
