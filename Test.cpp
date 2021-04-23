#include "HttpHeader.h"

#include <stdio.h>
#include <time.h>   
#include <iostream>

using namespace std;

int main() {
	HttpHeader h(Date());
	cout << h.getData();
	HttpHeader g(ContentLength(45));
	cout << g;
	HttpHeader s(Server(string("FORK YOU ASSHOLE!!!")));
	cout << s;
}