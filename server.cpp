#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <stdio.h>
#include <sys/wait.h>

using namespace std;

#define BACKLOG 5
#define BUFLEN 4096

class Server {
    int Server_fd;
    int Client_fd;
    int port;
    char request[BUFLEN];
public:
    Server(int portnum);
    ~Server() { close(Server_fd); }
    int get_fd() const { return Server_fd; }
    int get_client_fd() const {return Client_fd; }
    void Create_client();
    void Request();
    void Response(const char*, string);
};

Server::Server(int portnum) {
    port = portnum;
    if ((Server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Cant create socket" << endl;
        exit(2);
    }
    struct sockaddr_in ServAddr;
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_port = htons(port);
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(Server_fd, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) < 0) {
        cerr << "cant bind socket" << endl;
        close(Server_fd);
        exit(3);
    }
}

void Server::Create_client() {
    struct sockaddr_in ClientAddr;
    size_t ClAddrLen = sizeof(ClientAddr);
    Client_fd = accept(Server_fd, (struct sockaddr*)&ClientAddr, (socklen_t*)&ClAddrLen);
    if (Client_fd < 0) {
        cerr << "Client error" << endl;
        exit(5);
    }
}

int filetype(const char*req) {
    int i = 0;
    while (req[i] != '.' && req[i] != 0) {
        i++;
    }
    i++;
    char type[5];                           //Не рассматриваем разрешения длиннее 4.
    int k = 0;
    while (req[i] != 0 && (k < 4)) {
        type[k++] = req[i++];
    }
    type[k] = 0;
    if(!strcmp(type,"jpg")) return 1;
    if(!strcmp(type,"html")) return 2;
    return 0;
}

void Server::Response(const char* file, string header) {
    int fd;
    string str = header;
    str += "\nAllow: GET, HEAD\nServer: MyServer/1.1\nConnection: keep-alive";
//CONTENT LENGTH
    str += "\nContent-length: ";
    int len = 0;
    if (file != NULL) {
        fd = open(file, O_RDONLY);
        char c;
        while(read(fd, &c, 1)) len++;
        lseek(fd, 0, 0);
    }
    str += to_string(len);
//CONTENT TYPE
    if (file != NULL) {
        str += "\nContent-type: ";
        int type = filetype(file);
        if (type == 1) str += "image/jpeg";
        else if (type == 2) str += "text/html";
        else if (fd == -1) str += "text/html";          // если это ошибка
        else str += "text/plain";                       // это может быть server.cpp
    }
//DATE
    str += "\nDate: ";
    time_t now = time(0);
    str += ctime(&now);
//Last-modified
    if (file != NULL) {
        str += "Last-modified :";
        struct stat buff;
        fstat(fd, &buff);
        str += ctime(&buff.st_mtime);
    }
    str += "\n";                                        //Отделяем с помощью \n\n но первый \n заполнен от ctime
    //cout << str << endl;

    int n = str.length();
    char* buf = (char*)malloc(sizeof(char) * (n+1));
    strcpy(buf, str.c_str());
    len = strlen(buf);
    send(Client_fd, buf, len, 0);
    free(buf);

    if(!strncmp(request, "GET", 3)) {
        char buf2[BUFLEN];
        while((len = read(fd, buf2, BUFLEN)) > 0)
            send(Client_fd, buf2, len, 0);
        close(fd);
    }
}

void Server::Request() {
    int req = recv(Client_fd, request, BUFLEN, 0);
    if (req < 0) {
        cerr << "Server error" << endl;
        shutdown(Client_fd, SHUT_RDWR);
        close(Client_fd);
        close(Server_fd);
        exit(6);
    }
    if(req == 0) { return; }
    //cout << request << endl;

    if(strncmp(request, "GET", 3) && strncmp(request, "HEAD", 4)) {
        Response("501.html", "HTTP/1.1 501 NotImplemented");
        cerr << "Error: BadRequest" << endl;
    } else if(!strncmp(request, "HEAD", 4)) {
        //HEAD
        Response(NULL, "HTTP/1.1 200 MyServer");
    } else {
        //GET
        bool home = false;
        int i = 5;
        char c = request[i];
        while(c != ' ') {
            i++;
            c = request[i];  //пропускаем пробелы
        }
        char path[i-5];                    //возможный filename
        if (i == 5) {                      //URI не содержит filename
            path[0] = '/';
            path[1] = '\0';
            home = true;
        } else {                           //крпируем filename в path
            copy(&request[5], &request[i], &path[0]);
            path[i-5] = 0;
        }
        int Filefd = open(path, O_RDONLY);
        struct stat buff;
        fstat(Filefd, &buff);
        int tmp = Filefd;
        close(Filefd);
        bool IS_FILE = buff.st_mode & S_IFREG;
        if ((!home) && (!IS_FILE || (tmp == -1))) {          //не открывает файл
            Response("404.html", "HTTP/1.1 404 NotFound");
            cerr << "Error 404" << endl;
        } else {                                                        //если открыто или homepage
            if (home) Response("index.html", "HTTP/1.1 200 MyServer");
            else Response(path, "HTTP/1.1 200 MyServer");
        }
    }
}

int main(int argc, char**argv) {
    int portnum;
    if (argc == 2) {
        portnum = atoi(argv[1]);
    } else {
        cout << "I NEED PORT" << endl;
        exit(1);
    }
    Server server(portnum);
    int serv_fd = server.get_fd();
    if (listen(serv_fd, BACKLOG) < 0) {
        cerr << "can't listen" << endl;
        exit(4);
    }
    for(;;) {
        server.Create_client();
        server.Request();
        shutdown(server.get_client_fd(), SHUT_RDWR);
        close(server.get_client_fd());
    }
    return 0;
}
