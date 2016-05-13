#!/bin/bash

#SBATCH --job-name=HCLIB-PERFORMANCE-REGRESSION
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=12
#SBATCH --mem=48000m
#SBATCH --time=04:00:00
#SBATCH --mail-user=jmg3@rice.edu
#SBATCH --mail-type=ALL
#SBATCH --export=ALL
#SBATCH --partition=commons
#SBATCH --exclusive

set -e

if [[ -z "$RODINIA_DATA_DIR" ]]; then
    echo RODINIA_DATA_DIR must be set to the data directory of the Rodinia benchmark suite
    exit 1
fi

if [[ -z "$BOTS_ROOT" ]]; then
    echo BOTS_ROOT must be set to the root directory of the BOTS benchmark suite
    exit 1
fi

NTRIALS=10
if [[ $# -ge 1 ]]; then
    NTRIALS=$1
fi

DRY_RUN=0
if [[ $# -ge 2 ]]; then
    DRY_RUN=$2
fi

RUNNING_UNDER_SLURM=1
if [[ -z "$SLURM_JOB_ID" ]]; then
    echo Not executing under SLURM
    RUNNING_UNDER_SLURM=0
else
    echo Running under SLURM, cd-ing back to $SLURM_SUBMIT_DIR
    cd $SLURM_SUBMIT_DIR
fi

make clean
make -j

for FOLDER in $(ls rodinia/); do
    if [[ -d rodinia/$FOLDER ]]; then
        echo Compiling rodinia/$FOLDER
        cd rodinia/$FOLDER && make clean && make -f Makefile.ref clean && make && make -f Makefile.ref && cd ../../
    fi
done

for FOLDER in $(ls bots/); do
    if [[ -d bots/$FOLDER ]]; then
        echo Compiling bots/$FOLDER
        cd bots/$FOLDER && make clean && make -f Makefile.ref clean && make && make -f Makefile.ref && cd ../../
    fi
done

for FOLDER in $(ls kastors-1.1/); do
    if [[ -d kastors-1.1/$FOLDER && $FOLDER != common ]]; then
        echo Compiling kastor-1.1/$FOLDER
        cd kastors-1.1/$FOLDER && make clean && make -f Makefile.ref clean && make && make -f Makefile.ref && cd ../../
    fi
done

for FIX in 'bots/alignment_single/alignment.ref.icc.omp-tasks bots/alignment_single/alignment.icc.single-omp-tasks.ref' \
        'bots/fft/fft.ref.icc.omp-tasks bots/fft/fft.icc.omp-tasks.ref' \
        'bots/fib/fib.ref.icc.omp-tasks bots/fib/fib.icc.omp-tasks.ref' \
        'bots/floorplan/floorplan.ref.icc.omp-tasks bots/floorplan/floorplan.icc.omp-tasks.ref' \
        'bots/health/health.ref.icc.omp-tasks bots/health/health.icc.omp-tasks.ref' \
        'bots/nqueens/nqueens.ref.icc.omp-tasks bots/nqueens/nqueens.icc.omp-tasks.ref' \
        'bots/sort/sort.ref.icc.omp-tasks bots/sort/sort.icc.omp-tasks.ref' \
        'bots/sparselu_for/sparselu.ref.icc.omp-tasks bots/sparselu_for/sparselu.icc.for-omp-tasks.ref' \
        'bots/sparselu_single/sparselu.ref.icc.omp-tasks bots/sparselu_single/sparselu.icc.single-omp-tasks.ref' \
        'bots/strassen/strassen.ref.icc.omp-tasks bots/strassen/strassen.icc.omp-tasks.ref' \
        'bots/uts/uts.ref.icc.omp-tasks bots/uts/uts.icc.omp-tasks.ref'; do
    SRC=$(echo $FIX | awk '{ print $1 }')
    DST=$(echo $FIX | awk '{ print $2 }')
    mv $SRC $DST
done

echo Compilation completed!
echo
if [[ $DRY_RUN -eq 1 ]]; then
    exit 0
fi

MEDIAN_PY=../../../tools/median.py
MEAN_PY=../../../tools/mean.py
BENCHMARKS=("cilksort 100000000"
        "FFT 16384"
        "fib 45"
        "fib-ddt 45"
        "nqueens 14"
        "qsort 100000000"
        "rodinia/backprop/backprop 4194304"
        "rodinia/bfs/bfs 4 rodinia/bfs/graph1MW_6.txt"
        "rodinia/b+tree/b+tree.out core 2 file rodinia/b+tree/mil.txt command rodinia/b+tree/command.txt"
        "rodinia/cfd/euler3d_cpu_double rodinia/cfd/fvcorr.domn.193K"
        "rodinia/heartwall/heartwall rodinia/heartwall/test.avi 20 4"
        "rodinia/hotspot/hotspot 1024 1024 2 4 rodinia/hotspot/temp_1024 rodinia/hotspot/power_1024 output.out"
        "rodinia/hotspot3D/3D 512 8 100 rodinia/hotspot3D/power_512x8 rodinia/hotspot3D/temp_512x8 output.out"
        "rodinia/kmeans/kmeans -i $RODINIA_DATA_DIR/kmeans/kdd_cup"
        "rodinia/lavaMD/lavaMD -cores 4 -boxes1d 10"
        "rodinia/leukocyte/OpenMP/leukocyte 5 4 $RODINIA_DATA_DIR/leukocyte/testfile.avi"
        "rodinia/lud/omp/lud_omp -s 8000"
        "rodinia/nn/nn rodinia/nn/filelist_4 5 30 90"
        "rodinia/nw/needle 2048 10 2"
        "rodinia/particlefilter/particle_filter -x 128 -y 128 -z 10 -np 10000"
        "rodinia/pathfinder/pathfinder 100000 100"
        "rodinia/srad/srad 2048 2048 0 127 0 127 2 0.5 2"
        "rodinia/streamcluster/sc_omp 10 20 256 65536 65536 1000 none output.txt 4"
        "bots/alignment_for/alignment.icc.for-omp-tasks -f $BOTS_ROOT/inputs/alignment/prot.100.aa"
        "bots/alignment_single/alignment.icc.single-omp-tasks -f $BOTS_ROOT/inputs/alignment/prot.100.aa"
        "bots/fft/fft.icc.omp-tasks"
        "bots/fib/fib.icc.omp-tasks -n 30"
        "bots/floorplan/floorplan.icc.omp-tasks -f $BOTS_ROOT/inputs/floorplan/input.20"
        "bots/health/health.icc.omp-tasks -f $BOTS_ROOT/inputs/health/large.input"
        "bots/nqueens/nqueens.icc.omp-tasks -n 1000"
        "bots/sort/sort.icc.omp-tasks -n 100000000"
        "bots/sparselu_for/sparselu.icc.for-omp-tasks -n 50"
        "bots/sparselu_single/sparselu.icc.single-omp-tasks -n 50"
        "bots/strassen/strassen.icc.omp-tasks -n 4096"
        "bots/uts/uts.icc.omp-tasks -f $BOTS_ROOT/inputs/uts/small.input"
        "kastors-1.1/jacobi/jacobi-task -c -i 100"
        "kastors-1.1/jacobi/jacobi-block-task -c -i 100"
        "kastors-1.1/jacobi/jacobi-block-for -c -i 100")

TIMESTAMP=$(date +%s)
set +e
MACHINE=$(hostname -d)
if [[ -z "$MACHINE" ]]; then
    MACHINE=$(hostname)
fi
set -e
PATH=.:${PATH}

mkdir -p regression-logs-$MACHINE
if [[ $RUNNING_UNDER_SLURM == 1 ]]; then
    LOG_FILE=regression-logs-$MACHINE/$TIMESTAMP.dat
else
    LOG_FILE=regression-logs-$MACHINE/$TIMESTAMP.ignore
fi
REFERENCE_LOG_FILE=$(ls -lrt regression-logs-$MACHINE/ | grep dat | tail -n 1 | awk '{ print $9 }') 
touch $LOG_FILE

mkdir -p test_logs

for TEST in "${BENCHMARKS[@]}"; do
    TEST_EXE=$(echo $TEST | awk '{ print $1 }')
    TEST_ARGS=$(echo $TEST | awk '{ $1 = ""; print $0 }')
    TESTNAME=$(basename $TEST_EXE)
    TEST_LOG=test_logs/tmp.$TESTNAME.log
    REF_LOG=test_logs/tmp.$TESTNAME.ref.log

    echo "Running $TESTNAME from $(pwd), \"$TEST\""

    if [[ ! -f "$TEST_EXE" ]]; then
        echo Missing executable $TEST_EXE
    else
        REF=$TEST_EXE.ref

        for TRIAL in $(seq 1 $NTRIALS); do
            HCLIB_PROFILE_LAUNCH_BODY=1 $TEST 2>&1
        done > $TEST_LOG

        T=$(cat $TEST_LOG | grep 'HCLIB TIME' | awk '{ print $3 }'| python $MEAN_PY)

        if [[ ! -f "$REF" ]]; then
            echo Missing reference executable $REF
            echo $TESTNAME $T >> $LOG_FILE
        else
            for TRIAL in $(seq 1 $NTRIALS); do
                $REF $TEST_ARGS 2>&1
            done > $REF_LOG

            REF_T=$(cat $REF_LOG | grep 'HCLIB TIME' | awk '{ print $3 }' | python $MEAN_PY)
            echo $TESTNAME $T $REF_T >> $LOG_FILE
        fi
    fi
done

if [[ -z "$REFERENCE_LOG_FILE" || ! -f regression-logs-$MACHINE/$REFERENCE_LOG_FILE ]]; then
    echo No available reference peformance information
    exit 1
fi

while read LINE; do
    BENCHMARK=$(basename $(echo $LINE | awk '{ print $1 }'))
    NEW_T=$(echo $LINE | awk '{ print $2 }')
    OLD_T=$(cat regression-logs-$MACHINE/$REFERENCE_LOG_FILE | grep "^$BENCHMARK " | awk '{ print $2 }')
    if [[ -z "$OLD_T" ]]; then
        echo Unable to find an older run of \'$BENCHMARK\' to compare against
    else
        NEW_SPEEDUP=$(echo $OLD_T / $NEW_T | bc -l)
        echo $BENCHMARK $NEW_SPEEDUP
    fi
done < $LOG_FILE
