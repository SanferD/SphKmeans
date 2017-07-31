all:
	gcc sphkmeans.c utils.c wrappers.c types.c -DDEBUG -Wall -g -lm -o sphkmeans
kmeans:
	gcc sphkmeans.c utils.c wrappers.c types.c -Wall -lm -o sphkmeans
preprocess:
	python parser.py
clean:
	rm sphkmeans *.csv *.class *.clabel *.out
run:
	python run.py