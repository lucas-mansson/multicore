main:
	gcc -o preflow preflow.c -g -O3 -pthread
	time sh check-solution.sh ./preflow
	@echo PASS all tests

test: 
	gcc -o preflow preflow.c -g -O3 -pthread
	./preflow < ../../data/tiny/0.in

