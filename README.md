# dataflow_analysis


  opt -enable-new-pm=0 -load ./reaching.so --reaching ./test/reaching-test-m2r.bc -o out

  opt -enable-new-pm=0 -load ./liveness.so --liveness ./test/liveness-test-m2r.bc -o out