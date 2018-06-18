#include "utils.h"
#include "UtilSingleton.h"
#include "Perf.h"
#include <gtest/gtest.h>

using namespace utils;

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

TEST(Test_UtilSingleton, Singletone) {
    const string name = "create";
    auto pClass = UtilSingleton<MyClass>::GetInstance(name);
    auto pClass2 = UtilSingleton<MyClass>::GetInstance("create_test");
    ASSERT_EQ(name, pClass->getName());
    ASSERT_EQ(name, pClass2->getName());

    UtilSingleton<MyClass>::DesInstance();
}

TEST(Test_Perf, Perf) {
    Perf p("perfname");
    this_thread::sleep_for(std::chrono::seconds(2));
    p.done();

    Perf m("perfSelfDestroy");
    this_thread::sleep_for(std::chrono::seconds(2));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
