#DATA = ../data/railwayplanning/secret/3large.in
DATA = ../data/railwayplanning/secret/4huge.in
#DATA = ../data/big/002.in
#DATA = ../data/tiny/0.in
#DATA = ../data/railwayplanning/secret/1small.in
#DATA = ../data/huge/001.in

main:
	gcc -o preflow preflow.c pthread_barrier.c -g -O3 -pthread -latomic
	time sh check-solution.sh ./preflow
	@echo PASS all tests

test: 
	gcc -o preflow preflow.c -g -O3 -pthread -latomic
	time ./preflow < $(DATA) 

hg:
	gcc -o preflow preflow.c -g -O3 -pthread -latomic
	valgrind --tool=helgrind ./preflow < $(DATA)

tsan:
	clang -o preflow preflow.c -g -O3 -pthread -fsanitize=thread -latomic
	./preflow < $(DATA)

lab0:
	#gcc -o preflow_lab0 preflow_lab0.c -g -O3 -pthread -S
	gcc -o preflow_lab0 preflow_lab0.c -O0 -S
	time sh check-solution.sh ./preflow_lab0
	@echo PASS all tests
