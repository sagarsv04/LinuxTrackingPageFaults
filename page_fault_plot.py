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
