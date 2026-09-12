#DATA = ../../data/railwayplanning/secret/3large.in
DATA = ../../data/tiny/0.in

main:
	gcc -o preflow preflow.c -g -O3 -pthread
	time sh check-solution.sh ./preflow
	@echo PASS all tests

test: 
	gcc -o preflow preflow.c -g -O3 -pthread
	./preflow < $(DATA) 

hg:
	gcc -o preflow preflow.c -g -O3 -pthread
	valgrind --tool=helgrind ./preflow < $(DATA)

tsan:
	gcc -o preflow preflow.c -g -O3 -pthread -fsanitize=thread
	./preflow < $(DATA)
