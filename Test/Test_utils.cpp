#include "utils.h"
#include "UtilSingleton.h"
#include "perf.h"
#include <gtest/gtest.h>

using namespace utils;
#if 1
TEST(Test_launch_cmd, ShellCmd) {
    vector<string> output;
    ASSERT_EQ(0, launch_cmd("echo test", output));
    ASSERT_EQ(1, output.size());
    ASSERT_EQ("test", output[0]);

    output.clear();
    ASSERT_EQ(0, launch_cmd("", output));
    ASSERT_EQ(0, output.size());

    output.clear();
    ASSERT_EQ(0, launch_cmd("ls /dev/noexit", output));
    ASSERT_EQ(0, output.size());

    output.clear();
    ASSERT_EQ(0, launch_cmd("bash -c 'echo -e  \"test\\n12345\"'", output));
    ASSERT_EQ(2, output.size());
    ASSERT_EQ("test", output[0]);
    ASSERT_EQ("12345", output[1]);

    output.clear();
    ASSERT_EQ(0, launch_cmd("bash -c 'echo -e  \"test \\n 12345  \\n 12345 \\n  \\n\"'", output));
    ASSERT_EQ(3, output.size());
    ASSERT_EQ("test", output[0]);
    ASSERT_EQ("12345", output[1]);
    ASSERT_EQ("12345", output[2]);

    output.clear();
    ASSERT_EQ(0, launch_cmd("bash -c \"echo -e  \\\"test \\n 12345  \\n 12345 \\n  \\n\\\"\"", output));
    ASSERT_EQ(3, output.size());
    ASSERT_EQ("test", output[0]);
    ASSERT_EQ("12345", output[1]);
    ASSERT_EQ("12345", output[2]);
}

TEST(Test_get_current_mac_addrs, ShellCmd) {
    vector<string> macs = get_current_mac_addrs();
    for (auto&& s:macs)
        cout<<s<<endl;
}
#endif

#if 1
TEST(Test_split_by_delim, SplitString) {
    vector<string> strs = split_by_delim(":abc::efg:1234 5 ::6", ':');
    for(auto&& s: strs){
        cout<<s<<endl;
    }
    ASSERT_EQ(7, strs.size());
    ASSERT_EQ("", strs[0]);
    ASSERT_EQ("abc", strs[1]);
    ASSERT_EQ("", strs[2]);
    ASSERT_EQ("efg", strs[3]);
    ASSERT_EQ("1234 5 ", strs[4]);
    ASSERT_EQ("", strs[5]);
    ASSERT_EQ("6", strs[6]);
}

TEST(Test_std, StdExample) {
    string s = "a:b:c##@@c";
    //Searches the string for the first character that does not match any of the characters specified in its arguments.
    ASSERT_EQ(1, s.find_first_of(":"));
    ASSERT_EQ(5, s.find_first_of("#@"));

    s="#:$!abc";
    string p=s.substr(s.find_first_not_of(":$!#"));
    ASSERT_EQ("abc", p);
}
#endif

#if 1
TEST(Test_UtilSingleton, Singletone) {
    class MyClass {
        public:
            MyClass(const string& strData) : m_strData(strData) {
                cout << m_strData.data() << endl;
            };
            ~MyClass() {
                cout << "destory" << endl;
            };
            string getName(){
                return m_strData;
            }

        private:
            string m_strData;
    };


    const string name = "create";
    auto pClass = UtilSingleton<MyClass>::GetInstance(name);
    auto pClass2 = UtilSingleton<MyClass>::GetInstance("create_test");
    ASSERT_EQ(name, pClass->getName());
    ASSERT_EQ(name, pClass2->getName());

    UtilSingleton<MyClass>::DesInstance();
}

TEST(Test_perf, Perf) {
    perf p("perfname");
    this_thread::sleep_for(std::chrono::seconds(2));
    p.done();

    perf m("perfSelfDestroy");
    this_thread::sleep_for(std::chrono::seconds(2));
}
#endif

#if 1
#include <queue>
#include <vector>
#include <iostream>
#include "circ_buffer.h"

TEST(Test_circ_buffer, CircBuffer){
    int b = 0;
    circ_buffer<int> buf;
    buf.set_capacity(3);
    buf.send(1);
    buf.send(2);
    buf.send(3);
    buf.send(4);
    buf.send(5);
    buf.send(6);
    int s = buf.receive();
    ASSERT_EQ(s, 4);
    s = buf.receive();
    ASSERT_EQ(s, 5);
    s = buf.receive();
    ASSERT_EQ(s, 6);

    buf.send(9);
    s = buf.receive();
    ASSERT_EQ(s, 9);
}
#endif

TEST(Test_STLAlgo, Algo)
{
    //for_each
    {
        cout<<"for_each"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        vector<int> vj;
        for_each(vi.begin(),vi.end(),[&vj](int& v){cout<<v<<endl; vj.push_back(v);});
        for (size_t i=0;i<vj.size();i++){
            ASSERT_EQ(vj[i], vi[i]);
        }

        vj.clear();
        for_each(vi.begin(),vi.begin()+5,[&vj](int& v){cout<<v<<endl; vj.push_back(v);});
        ASSERT_EQ(vj.size(), 5);
        for (size_t i=0;i<vj.size();i++){
            ASSERT_EQ(vj[i], vi[i]);
        }

        struct myclass {           // function object type:
            void operator() (int i) {std::cout << ' ' << i;}
        } myobject;
        for_each(vi.begin(), vi.end(), myobject);
        cout<<endl;
    }

    //equal
    {
        /*
         * The overload of operator == that works on two std::vectors will compare the vector sizes and return false if those are different;
         * if not, it will compare the contents of the vector element-by-element.
         * If operator == is defined for the vector's element type, then the comparison of vectors through operator == is valid and meaningful.
         */
        cout<<"equal"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        vector<int> vj={1,2,3,4,5,6,7};
        ASSERT_EQ(vi, vj);
        ASSERT_EQ(vi == vj, true);
        ASSERT_EQ(equal(vi.begin(), vi.end(), vj.begin()), true);

        /*
        ASSERT_EQ(equal(vi.begin(), vi.end(), vj.begin(), ([](const int& i, const int& j)->bool{
            return i==j;
        })), true);
        */
        string s1="abcd";
        string s2="ABCD";
        bool ret = equal(vi.begin(), vi.end(), vj.begin(), [](const char& i, const char& j)->bool{
            return toupper(i) == toupper(j);
        });

        ASSERT_EQ(ret, true);

        vj.push_back(5);
        ASSERT_NE(vi == vj, true);

        if (vj.size() >= vi.size())
            ASSERT_EQ(equal(vi.begin(), vi.end(), vj.begin()), true);
        else
            ASSERT_EQ(equal(vj.begin(), vj.end(), vi.begin()), true);

    }

    //reverse
    {
        cout<<"reverse"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        reverse(vi.begin(), vi.end());
        for_each(vi.begin(), vi.end(), [](int& v){ cout<<' '<<v;});
        cout<<endl;
    }

    //reverse_copy
    {
        cout<<"reverse_copy"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        vector<int> vj;
        vj.resize(vi.size());
        reverse_copy(vi.begin(), vi.end(), vj.begin());
        for_each(vj.begin(), vj.end(), [](int& v){ cout<<' '<<v;});
        cout<<endl;
    }

    //swap
    {
        cout<<"swap"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        vector<int> vi_record=vi;
        vector<int> vj={8,9,10};
        swap(vi,vj);

        ASSERT_EQ(vj.size(), vi_record.size());
        for (size_t i=0;i<vj.size();i++){
            ASSERT_EQ(vj[i], vi_record[i]);
        }
    }

    //copy
    {
        cout<<"copy"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        vector<int> vj(7);
        copy(vi.begin(), vi.end(), vj.begin());
        ASSERT_EQ(equal(vi.begin(), vi.end(), vj.begin()), true);
    }

    //copy_n
    {
        cout<<"copy_n"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        vector<int> vj(7);
        copy_n(vi.begin(), 5, vj.begin());
        ASSERT_EQ(equal(vi.begin(), vi.begin()+5, vj.begin()), true);
    }

    //fill
    {
        cout<<"fill"<<endl;
        vector<int> vi(20);
        fill(vi.begin(), vi.end(), 5);
        for_each(vi.begin(),vi.end(),[](int& v){ ASSERT_EQ(v, 5);});
    }
    //fill_n
    {
        cout<<"fill_n"<<endl;
        vector<int> vi(20);
        fill_n(vi.begin(), 3, 5);
        for_each(vi.begin(),vi.begin()+3,[](int& v){ ASSERT_EQ(v, 5);});
        ASSERT_EQ(vi[4],0);
    }
    //random_shuffle
    {
        cout<<"random_shuffle"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        random_shuffle(vi.begin(), vi.end());
        for_each(vi.begin(), vi.end(), [](int& v){cout<<' '<<v;});
        cout<<endl;
    }
    //rotate
    {
        //Rotates the order of the elements in the range [first,last), in such a way
        //that the element pointed by middle becomes the new first element.

        cout<<"rotate"<<endl;
        std::vector<int> vi;

        // set some values:
        for (int i=1; i<10; ++i) vi.push_back(i); // 1 2 3 4 5 6 7 8 9

        std::rotate(vi.begin(),vi.begin()+3,vi.end()); // 4 5 6 7 8 9 1 2 3
        // print out content:
        std::cout << "vi contains:";
        for (std::vector<int>::iterator it=vi.begin(); it!=vi.end(); ++it)
            std::cout << ' ' << *it;
        std::cout << '\n';

        ASSERT_EQ(vi[0] == 4, true);
        ASSERT_EQ(vi[1] == 5, true);
    }
    //mismatch
    {
        cout<<"mismatch"<<endl;
        vector<int>vi={1,2,3,4,5};
        vector<int>vj={1,2,3,9,7};
        auto pair = mismatch(vi.begin(), vi.end(), vj.begin());

        ASSERT_EQ(*pair.first, 4);
        ASSERT_EQ(*pair.second, 9);
        cout<<"the first mismatch value:"<<*pair.first<<" " <<*pair.second<<endl;

        auto itor2 = mismatch(vi.begin(), vi.end(), vj.begin(), [](int&i, int&j){ return i==j;});
        ASSERT_EQ(*itor2.first, 4);
        ASSERT_EQ(*itor2.second, 9);
        cout<<"the first mismatch value:"<<*itor2.first<<" " <<*itor2.second<<endl;
    }
    //find
    {
        cout<<"find & distance"<<endl;
        vector<int>vi={1,2,3,4,3,5};
        auto itor = find(vi.begin(), vi.end(), 3);
        ASSERT_EQ(*itor, 3);
        //ASSERT_EQ(distance(vi.begin(), itor), 3);
        ASSERT_EQ(distance(vi.begin(), itor), 2);

        auto itor2 = find(vi.begin(), vi.end(), 9);
        ASSERT_EQ(itor2, vi.end());
    }

    //find_if
    {
        vector<int>vi={1,2,3,4,3,5};
        auto itor = find_if(vi.begin(), vi.end(), [](int&v){return v>3;});
        ASSERT_EQ(distance(vi.begin(),itor), 3);
    }
    //find_first_of
    {
        /*
         * Find element from set in range
         * Returns an iterator to the first element in the range [first1,last1) that matches any of the elements
         * in [first2,last2). If no such element is found, the function returns last1.
         * The elements in [first1,last1) are sequentially compared to each of the values in [first2,last2)
         * using operator== (or pred, in version (2)), until a pair matches.
         */
        cout<<"find_first_of"<<endl;
        int mychars[] = {'a','b','c','A','B','C'};
        std::vector<char> haystack (mychars,mychars+6);
        std::vector<char>::iterator it;

        int needle[] = {'A','B','C'};

        // using default comparison:
        it = find_first_of (haystack.begin(), haystack.end(), needle, needle+3);

        if (it!=haystack.end())
            std::cout << "The first match is: " << *it << '\n';

        // using predicate comparison:
        it = find_first_of (haystack.begin(), haystack.end(),
                            needle, needle+3, [](char c1, char c2){
                                return (std::tolower(c1)==std::tolower(c2));
                            });

        if (it!=haystack.end())
            std::cout << "The first match is: " << *it << '\n';
    }
    //search
    {
        cout<<"search"<<endl;
        vector<int>vi={1,2,3,4,5,6};
        vector<int>vj={3,4,5};
        auto itor = search(vi.begin(), vi.end(), vj.begin(), vj.end());
        ASSERT_EQ(distance(vi.begin(), itor), 2);
        ASSERT_EQ(*itor, 3);

        string s1="abcdEfGhij";
        string s2="EFG";
        auto itor2 = search(s1.begin(), s1.end(), s2.begin(), s2.end(), [](char a, char b){
            return toupper(a)==toupper(b);
        });
        ASSERT_EQ(distance(s1.begin(), itor2), 4);
        cout<<*itor2<<endl;
    }
    //-------------------------------------------------------------
    //remove
    {
        cout<<"remove"<<endl;
        int myints[] = {10,20,30,30,20,10,10,20};      // 10 20 30 30 20 10 10 20

        // bounds of range:
        int* pbegin = myints;                          // ^
        int* pend = myints+sizeof(myints)/sizeof(int); // ^                       ^

        pend = std::remove (pbegin, pend, 20);         // 10 30 30 10 10 ?  ?  ?
                                                       // ^              ^
        std::cout << "range contains:";
        for (int* p=pbegin; p!=pend; ++p)
            std::cout << ' ' << *p;
        std::cout << '\n';
    }
    //remove_if
    {
        cout<<"remove_if"<<endl;
        int myints[] = {1,2,3,4,5,6,7,8,9};            // 1 2 3 4 5 6 7 8 9

        // bounds of range:
        int* pbegin = myints;                          // ^
        int* pend = myints+sizeof(myints)/sizeof(int); // ^                 ^

        pend = std::remove_if (pbegin, pend, [](int&v){return (v%2) == 1;});   // 2 4 6 8 ? ? ? ? ?
                                                                              // ^       ^
        std::cout << "the range contains:";
        for (int* p=pbegin; p!=pend; ++p)
            std::cout << ' ' << *p;
        std::cout << '\n';

    }
    //remove_copy
    {
        int myints[] = {10,20,30,30,20,10,10,20};               // 10 20 30 30 20 10 10 20
        std::vector<int> myvector (8);

        std::remove_copy (myints,myints+8,myvector.begin(),20); // 10 30 30 10 10 0 0 0

        std::cout << "myvector contains:";
        for (std::vector<int>::iterator it=myvector.begin(); it!=myvector.end(); ++it)
            std::cout << ' ' << *it;
        std::cout << '\n';

    }
    //remove_copy_if
    {
        int myints[] = {1,2,3,4,5,6,7,8,9};
        std::vector<int> myvector (9);

        std::remove_copy_if (myints,myints+9,myvector.begin(), [](int&v){return (v%2) == 1;});   // 2 4 6 8 ? ? ? ? ?

        std::cout << "myvector contains:";
        for (std::vector<int>::iterator it=myvector.begin(); it!=myvector.end(); ++it)
            std::cout << ' ' << *it;
        std::cout << '\n';
    }
    //-------------------------------------------------------------
    //replace
    {
        /*
         * Assigns new_value to all the elements in the range [first,last) that compare equal to old_value.
         * The function uses operator== to compare the individual elements to old_value.
         */
        cout<<"replace"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        replace(vi.begin(), vi.end(), 5, 9);
        ASSERT_EQ(vi[4], 9);
    }
    //replace_if
    {
        cout<<"replace"<<endl;
        vector<int> vi={1,2,3,4,5,6,7};
        replace_if(vi.begin(), vi.begin()+5, [](int& v){return v<=5;} , 9);
        for_each(vi.begin(), vi.begin()+5, [](int&v){ASSERT_EQ(v, 9);});
    }
    //replace_copy
    //replace_copy_if
    {
        //Copies the elements in the range [first,last) to the range beginning at result,
        //replacing those for which pred returns true by new_value.

        std::vector<int> foo,bar;

        // set some values:
        for (int i=1; i<10; i++) foo.push_back(i);          // 1 2 3 4 5 6 7 8 9

        bar.resize(foo.size());   // allocate space
        std::replace_copy_if (foo.begin(), foo.end(), bar.begin(), [](int&v){return (v%2)==1;}, 0); // 0 2 0 4 0 6 0 8 0

        std::cout << "bar contains:";
        for (std::vector<int>::iterator it=bar.begin(); it!=bar.end(); ++it)
            std::cout << ' ' << *it;
        std::cout << '\n';

    }
    //-------------------------------------------------------------
    //count
    {
        cout<<"count"<<endl;
        vector<int>vi={1,2,3,4,3,6};
        int num = count(vi.begin(), vi.end(), 3);
        ASSERT_EQ(num, 2);
    }
    //count_if
    {
        cout<<"count_if"<<endl;
        vector<int>vi={1,2,3,4,3,6};
        int num = count_if(vi.begin(), vi.end(), [](int& v){ return v>3;});
        ASSERT_EQ(num, 2);
    }


}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
