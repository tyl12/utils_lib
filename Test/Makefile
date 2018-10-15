#ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1
#CFLAGS_KASAN+=$(call cc-option, -fno-sanitize-address-use-after-scope)
#CFLAGS_KASAN+=$(call cc-option, -fsanitize-address-use-after-scope)
#CFLAGS += -fsanitize-address-use-after-scope
#$(info $(CFLAGS_KASAN))


test:
	#g++-8.2 -std=c++14 test.cpp -lpthread -fsanitize=address
	g++-8.2 -std=c++14 test.cpp -lpthread -fsanitize-address-use-after-scope -fsanitize=address
	./a.out
	#ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) ./a.out

all:
	g++-8.2 -std=c++11 test.cpp -lpthread $(CFLAGS) $(CFLAGS_KASAN)  -llsan
	./a.out

.PHONY: using-gcc using-gcc-static using-clang

using-gcc:
	g++-8.2 -std=c++11 -o main-gcc -lasan -lpthread -O -g -fsanitize=address -fno-omit-frame-pointer   -fsanitize-address-use-after-scope    main.cpp && \
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) ./main-gcc

using-gcc-static:
	g++-8.2 -std=c++11 -o main-gcc-static -static-libstdc++ -static-libasan  -lpthread -O -g -fsanitize=address -fno-omit-frame-pointer main.cpp && \
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) ./main-gcc-static

using-clang:
	clang -o main-clang -x c++ -O -g -fsanitize=address main.cpp && \
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) ./main-clang
