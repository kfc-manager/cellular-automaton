#!/bin/bash

#SBATCH --job-name=cellular_automaton
#SBATCH --output=cellular_automaton-%j.out
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --time=00:20:00
#SBATCH --reservation=pr

set -e

# srun ./caseq 1024 0
mpirun ./solution 1024 0