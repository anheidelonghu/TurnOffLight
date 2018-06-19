#include <stdio.h>
#include <chrono>
#include <iostream>
using namespace std;
using ns = chrono::nanoseconds;
using get_time = chrono::steady_clock;

int main()
{
	auto start = get_time::now();
	cout << "hello" << endl;
	auto end = get_time::now();
	auto diff = end - start;
	cout << chrono::duration_cast<ns>(diff).count() << endl;
	return 0;
}