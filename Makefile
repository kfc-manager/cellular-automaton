CFLAGS=-O2
LDFLAGS=-lcrypto

.PHONY: clean

solution: solution.c random.c md5tool.c
	mpicc $(CFLAGS) $^ $(LDFLAGS) -o $@

caseq: caseq.c random.c md5tool.c
	gcc $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -rf *.o
	rm solution
	rm *.out

run: solution caseq
	sbatch script.sh

