  mkdir -p _out/avx2/rel
  mkdir -p _out/sse4/rel
  cc -o _out/avx2/rel/zval.o -std=gnu11 -march=native -Wall -Wextra -DAVX2 -O3 -c z_validate.c 
  cc -o _out/sse4/rel/zval.o -std=gnu11 -march=native -Wall -Wextra -DSSE4 -O3 -c z_validate.c 
  cc -shared -o _out/avx2/rel/zval.so _out/avx2/rel/zval.o 
  cc -shared -o _out/sse4/rel/zval.so _out/sse4/rel/zval.o 
