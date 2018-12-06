
void testfunc( int (*inf)(int, int))
{
    inf(1,2);
}
void testfunc2( int inf(int, int))
{
    inf(1,2);
}
