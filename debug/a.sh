gcc -std=c++11 -c -Wall -Werror -fPIC UtilSingleton.cpp
gcc -std=c++11 -shared  -fvisibility=default -o libutils.so UtilSingleton.o
nm libutils.so | grep -i "single"

g++ -std=c++11 -c -Wall -Werror -fPIC test.cpp # -fvisibility=default  
g++ -std=c++11 -shared  -o libtest.so test.o  # -fvisibility=default  
nm libtest.so | grep -i "testclass"
nm libtest.so | grep -i "testfunc"

g++ -std=c++11 -o a.out main.cpp -L. -ltest
./a.out



g++ -std=c++11 -c -Wall -Werror -fPIC UtilSingleton.cpp # -fvisibility=default  
g++ -std=c++11 -shared  -o libutils.so UtilSingleton.o  # -fvisibility=default  
nm libutils.so | grep -i "util"
nm libutils.so | grep -i "Singleton"
