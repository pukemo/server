#include "HttpHeader.h"
#include "HttpResponse-Request-Code.h"
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
//#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>


using namespace std;

int main() {
	//const char request[] = ;
	HttpRequest req("GET \n");
	HttpResponse resp(0, req);
	cout << req.is_correct << endl;

	return 0;
}