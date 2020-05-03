#!/bin/python
import os
import sys
import resource
import time

def page_fault():

	if len(sys.argv) != 2:
		print("Usage: python",sys.argv[0],"<number of desired page faults>")
		return -1

	N = int(sys.argv[1])
	# N = 10
	print("Page size:", resource.getpagesize())
	print("Size of integer:", sys.getsizeof(N))

	ints_per_page = int(resource.getpagesize() / sys.getsizeof(N)) #
	arr = [0] * ints_per_page * N

	i = 0
	while i < len(arr):
		# print("arr[" + str(i) + "]: " + str(arr[i])) # Slow version
		i += ints_per_page # skipping over an entire page, thus causing a minor page fault
		# i += 1;
		time.sleep(1)

	print("My Work is done")
	return 0

def main():
	print("My Pid :: {0}".format(os.getpid()))
	time.sleep(12)
	page_fault()
	return 0

if __name__ == "__main__":
	main()
