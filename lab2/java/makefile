#DATA = ../../data/railwayplanning/secret/2med.in
#DATA = ../../data/railwayplanning/secret/3large.in
#DATA = ../../data/railwayplanning/secret/4huge.in
#DATA = ../../data/big/002.in
DATA = ../../data/tiny/0.in

java:
	javac parallel.java
	time sh check-solution.sh java Preflow
	@echo PASS all tests

test:
	javac parallel.java
	java Preflow $(DATA)
	@echo PASS all tests

seq:
	javac parallel.java
	time sh check-solution.sh java Preflow
	@echo PASS all tests

one: 
	javac sequential.java
	java Preflow < i
