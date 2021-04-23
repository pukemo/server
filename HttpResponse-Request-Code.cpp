#define _CRT_NONSTDC_NO_DEPRECATE
#include "HttpHeader.h"
#include "HttpResponse-Request-Code.h"
#include <iostream>
#include <string.h>
//#include <unistd.h>
#include <sys/types.h>
#include <magic.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <io.h>

using namespace std;

enum {
	OK = 200,
	BAD_REQUEST = 400,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	INT_SERV_ERR = 500,
	NOT_IMPL = 501,
	SERV_UNAV = 503,

	OK_HEAD = 5,
	BAD_REQ_HEAD = 4,
	FORB_HEAD = 4,
	NOT_FOUND_HEAD = 4,
	INT_SERV_ERR_HEAD = 4,
	NoT_IMPL_HEAD = 5,
	SERV_UNAV_HEAD = 4,

	NUM_ALLOWED = 1,

	HTTP_LENGTH = 8,//    HTTP/1.0
	GET_LENGTH = 3,
	MIN_REQ_LENGTH = 15, // GET / HTTP/1.0\n

	METHOD_ERR = 1,
	SYNTAX_ERR = 2,
	HOME_PAGE = 3,
	NORMAL_WORK = 4,

	PLAIN = 0,
	HTML = 1,
	JPEG = 2,

	DATEBUFF = 80
};

//const string base_headers = { "Date", "Server", "Content-type", "Content-length" };
//const string ok_headers = { "Date", "Server", "Content-type", "Content-length", "Last-modified" };
//const string not_impl_headers = { "Date", "Server", "Allow", "Content-type", "Content-length" };
string allowed_commands[] = { string("GET") };
const string serv = "Model HTTP Server/0.1";

//const string comments = { "OK", "Bad request", "Forbidden", "Not Found", "Internal Server Error", "Not Implemented", "Service Unavailable", "Code Error" };

Code::Code() {
	code = 0;
	num_headers = 0;
	headers = nullptr;
}

Code::Code(int c) {
	this->headers = nullptr;
	this->num_headers = 0;
	this->code = c;
	switch (c) {
	case OK:
		this->comment = "OK";
		this->num_headers = OK_HEAD;
		break;
	case BAD_REQUEST:
		this->comment = "Bad request";
		this->num_headers = BAD_REQ_HEAD;
		break;
	case FORBIDDEN:
		this->comment = "Forbidden";
		this->num_headers = FORB_HEAD;
		break;
	case NOT_FOUND:
		this->comment = "Not Found";
		this->num_headers = NOT_FOUND_HEAD;
		break;
	case INT_SERV_ERR:
		this->comment = "Internal Server Error";
		this->num_headers = INT_SERV_ERR_HEAD;
		break;
	case NOT_IMPL:
		this->comment = "Not Implemented";
		this->num_headers = NoT_IMPL_HEAD;
		break;
	case SERV_UNAV:
		this->comment = "Service Unavailable";
		this->num_headers = SERV_UNAV_HEAD;
		break;
	default:
		this->comment = "Code Error";
		break;
	}
}

void Code::SetCode(int c) {
	switch (c) {
	case OK:
		this->comment = "OK";
		this->num_headers = OK_HEAD;
		break;
	case BAD_REQUEST:
		this->comment = "Bad request";
		this->num_headers = BAD_REQ_HEAD;
		break;
	case FORBIDDEN:
		this->comment = "Forbidden";
		this->num_headers = FORB_HEAD;
		break;
	case NOT_FOUND:
		this->comment = "Not Found";
		this->num_headers = NOT_FOUND_HEAD;
		break;
	case INT_SERV_ERR:
		this->comment = "Internal Server Error";
		this->num_headers = INT_SERV_ERR_HEAD;
		break;
	case NOT_IMPL:
		this->comment = "Not Implemented";
		this->num_headers = NoT_IMPL_HEAD;
		break;
	case SERV_UNAV:
		this->comment = "Service Unavailable";
		this->num_headers = SERV_UNAV_HEAD;
		break;
	default:
		this->comment = "Code Error";
		break;
	}
}

/*Code::Code(Code& c) {
	this = &c;
}*/

Code::~Code() {
	if (headers) {
		free(headers);
	}
}

void AddHeader(Code code, HttpHeader newh) {
	code.num_headers++;
	if (!code.headers) {
		code.headers = (HttpHeader**)malloc(sizeof(HttpHeader*));
	}
	else {
		code.headers = (HttpHeader**)realloc(code.headers, sizeof(HttpHeader*) * code.num_headers);
	}
	HttpHeader tmp(newh);
	code.headers[code.num_headers - 1] = &tmp;
}

ostream& operator<<(std::ostream& os, const Code& c) {
	os << c.code << ": " << c.comment;
	for (int i = 0; i < c.num_headers; i++) {
		os << *(c.headers[i]);
	}
	return os;
}

Param::Param(string name, string value) {
	this->name = string(name);
	this->value = string(value);
}

HttpRequest::HttpRequest() {
	is_correct = 0;
	method = 0;
	is_home_page = 0;
	params = nullptr;
	num_params = 0;
}

HttpRequest::HttpRequest(const char request[]) {
	is_correct = 0;
	method = 0;
	is_home_page = 0;
	params = nullptr;
	num_params = 0;
	try {
		string req(request);

		this->method = 0;
		this->is_correct = 0;
		this->is_home_page = 0;

		if (req.compare(0, GET_LENGTH, "GET")) {
			throw METHOD_ERR;
		}
		if (req.length() < MIN_REQ_LENGTH) {
			throw SYNTAX_ERR;
		}

		int par_start;
		int http_start;
		int http_end;
		int path_start = GET_LENGTH;

		while (req[path_start] == ' ' && path_start < (int)req.length()) {
			path_start++;
		}
		if (path_start == req.length() || path_start == GET_LENGTH) {
			throw SYNTAX_ERR;
		}

		switch (req[path_start]) {
		case '\n': // GET    \n...
		case '?':  // GET     ?...
		case '&':  // GET     &...
		case '=':  // GET     =...
		//case '%':
		//case '@':
		//case'#':
		//case '$':
			throw SYNTAX_ERR;
			break;
		case 'H':   // GET     H
			if (path_start + HTTP_LENGTH >= (int)req.length()) { // не влезает даже HTTP/1.0
				throw SYNTAX_ERR;
				break;
			}
			if (!req.compare(path_start, HTTP_LENGTH, "HTTP/1.0")) { // GET      HTTP/1.0
				throw SYNTAX_ERR;
				break;
			}
		default:
			if ((http_start = req.rfind("HTTP/1.0", req.length() - 1)) == -1) { // не нашли HTTP/1.0
				throw SYNTAX_ERR;
				break;
			}
			http_end = http_start + HTTP_LENGTH;
			int is_endl = 0;
			while (http_end < (int)req.length() && (req[http_end] == ' ' || req[http_end] == '\n')) {
				if (req[http_end] == '\n') is_endl = 1;
				http_end++;
			}
			if (http_end != req.length() || (http_end == req.length() && !is_endl)) { // после HTTP/1.0 есть что-то, кроме ' ' и '\n', или нет '\n'
				throw SYNTAX_ERR;
			}
			if (req[http_start - 1] != ' ') {   // перед HTTP/1.0 не пробел
				throw SYNTAX_ERR;
			}
			break;
		}

		int test_endl = 0;
		while (test_endl < http_start && req[test_endl] != '\n') {
			test_endl++;
		}
		if (test_endl != http_start) { // до HTTP/1.0 есть '\n'
			throw SYNTAX_ERR;
		}

		//GET   path_start   http_start
		//GET /vcdcv/sasvxz_/@%/grehdstvcrcs?wfauvb=ewfgashchb2&   vweudahvuyasd=wjffaksdjvjsc& ...ighh=fqh HTTP/1.0   \n  \n
		//GET /vcdcv/sasvxz_/@%/grehdstvcrcs     HTTP/1.0   \n  \n
		this->num_params = 0;
		if ((par_start = req.find("?", 0)) == -1) {// no paramethers
			int path_end = http_start - 1;

			while (path_end != path_start || req[path_end] == ' ') {
				path_end--;
			}
			if (path_start == path_end) {
				if (req[path_start] == '/') {
					//вызываем домашнюю страницу
					throw HOME_PAGE;
				}
				else {
					this->path.assign(req, path_start, 1);
					throw NORMAL_WORK;
				}
			}
			int restricted = req.find(" ", path_start);
			if (restricted > 0 && restricted < path_end) {
				throw SYNTAX_ERR;
			}
			restricted = req.find("?", path_start);
			if (restricted > 0 && restricted < path_end) {
				throw SYNTAX_ERR;
			}
			restricted = req.find("&", path_start);
			if (restricted > 0 && restricted < path_end) {
				throw SYNTAX_ERR;
			}
			this->path.assign(req, path_start, path_end - path_start + 1);
		}
		else { // found '?'
			if (req.find("?", par_start + 1) != -1) { // больше одного '?' в запросе
				throw SYNTAX_ERR;
			}
			/*if (par_star + 1 == http_start) { // ...?      HTTP/1.0
				throw SYNTAX_ERR;
			}*/
			switch (req[par_start + 1]) {
			case ' ':   // ...? ...
			case '&':   // ...?&...
			case '?':   // ...??...
			case '=':   // ...?=...
			//case '-':
			//case '+':
			//case '*':
			//case '<':
			//case '>':
			//case '$':
			//case '#':
			//case '@':
				throw SYNTAX_ERR;
				break;
			default:
				break;
			}
			int test_path = path_start;
			while (test_path < par_start && req[test_path] != ' ') {
				test_path++;
			}
			if (test_path < par_start) { // посреди пути есть пробелы или пробел между path и ?
				throw SYNTAX_ERR;
			}
			// в норме от path_start и до '?' нет пробелов
			this->path.assign(req, path_start, par_start - path_start);

			int par_search = par_start + 1;
			int cur_par = par_start + 1;
			int is_par = 1;  // ... name=value&
			int is_value = 0; // ...value&
			string par_name;
			string par_value;
			while (par_search < http_start - 1) {
				switch (req[par_search]) {
				case '=':
					if (!is_par || is_value) { // name=value & =        name=val=
						throw SYNTAX_ERR;
					}
					par_name.assign(req, cur_par, par_search - cur_par);
					cur_par = par_search + 1;
					is_value = 1;
					break;
				case '&':
					if (is_par && !is_value) { // na&me=value
						throw SYNTAX_ERR;
					}
					if (!is_par) { // & &
						throw SYNTAX_ERR;
					}
					if (cur_par == par_search) {// name=&
						throw SYNTAX_ERR;
					}
					par_value.assign(req, cur_par, par_search - cur_par);
					if (this->num_params) {
						this->params = (Param**)realloc(params, sizeof(Param*) * num_params);
					}
					else {
						this->params = (Param**)malloc(sizeof(Param*));
					}
					params[num_params] = new Param(par_name, par_value);
					num_params++;
					par_name.erase();
					par_value.erase();
					is_par = 0;
					is_value = 0;
					cur_par = par_search + 1;
					break;
				case ' ':
					// na me=value&   name =value&
					// name=val ue&    name=value &     name= value&
					if (is_par) {
						throw SYNTAX_ERR;
					}
					//     ...&     name=value&...
					cur_par = par_search;
					break;
				default:
					if (!is_par) {
						cur_par = par_search;
						is_par = 1;
					}
					break;
				}
				par_search++;
			}
			if (!(this->num_params)) { //есть ?, но нет параметров
				throw SYNTAX_ERR;
			}
			if (!is_par || !is_value) {// name=value& HTTP/1.0        name HTTP/1.0     
				throw SYNTAX_ERR;
			}
		}
		throw NORMAL_WORK;
	} 
	catch (const int& err) {
		switch (err) {
		case NORMAL_WORK:
			break;
		case HOME_PAGE:
			this->is_home_page = 1;
			//this->path = "название файла"
			break;
		case SYNTAX_ERR:
			this->is_correct = 1;
			break;
		case METHOD_ERR:
			this->method = 1;
			break;
		default:
			break;
		}
	}
}


HttpResponse::HttpResponse(int flag, HttpRequest& req) {
	code = Code();
	fd = 0;
	try {
		if (flag) {
			throw flag;
		}
		if (req.method) {
			throw NOT_IMPL;
		}
		if (req.is_correct) {
			throw BAD_REQUEST;
		}
		if (req.is_home_page) {
			throw OK;
		}
		struct stat stbuf;
		if (stat(req.path.c_str(), &stbuf) < 0) {
			throw NOT_FOUND;
		}
		int mode_check = 0;
		//stbuf.st_mode sgtrwxrwxrwx   rwx1 - uid, rwx2 - gid, rwx3 - other
		/*if (stbuf.st_uid == getuid()) {
			//700   ---111------    111000000
			//mode_check = 448;
			// ---100------  100000000
			mode_check = 256;
		}
		else if (stbuf.st_gid == getgid()) {
			//70 ------111---    111000
			//mode_check = 56;
			// ------100---   100000
			mode_check = 32;
		}
		else {*/
			//7    ---------111    111
			//mode_check = 7;
			// ---------100 100
			mode_check = 4;
		//}
		if (stbuf.st_mode & mode_check) {
			throw OK;
		}
		else {
			throw FORBIDDEN;
		}

	}
	catch (const int& res_code) {
		string date = string();
		int len = 0;
		switch (res_code) {
		case OK:
			//"Date", "Server", "Content-type", "Content-length", "Last-modified"
			this->code.SetCode(OK);
			struct stat stbuf;
			stat(req.path.c_str(), &stbuf);
			//open file

			//change len
			len = stbuf.st_size;
			//change date according to stat
			char buffer[DATEBUFF];
			strftime(buffer, sizeof(buffer), "%a, %d %b %G %H:%M:%S GMT", localtime(&stbuf.st_mtime));
			//fill Code
			//int type = PLAIN;
			
			fd = open(req.path.c_str(), O_RDONLY);
			magichandle_t* mh;
			mh = magic_init(MAGIC_FLAGS_NONE);
			magic_read_entries(mh, "magic");

			//change type according to file type
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			AddHeader(this->code, ContentType(magic_identify_file(mh, req.path.c_str())));
			AddHeader(this->code, ContentLength(len));
			AddHeader(this->code, LastModified(buffer));
			//show the content of file
			break;
		case BAD_REQUEST:
			//"Date", "Server"
			this->code.SetCode(BAD_REQUEST);
			//fill Code
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			break;
		case FORBIDDEN:
			//"Date", "Server"
			this->code.SetCode(FORBIDDEN);
			//fill Code
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			break;
		case NOT_FOUND:
			//"Date", "Server"
			this->code.SetCode(NOT_FOUND);
			//fill Code
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			break;
		case INT_SERV_ERR:
			//"Date", "Server"
			this->code.SetCode(INT_SERV_ERR);
			//fill Code
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			break;
		case NOT_IMPL:
			//"Date", "Server", "Allow"
			this->code.SetCode(NOT_IMPL);
			//fill Code
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			AddHeader(this->code, Allow(allowed_commands, NUM_ALLOWED));
			break;
		case SERV_UNAV:
			//"Date", "Server"
			this->code.SetCode(SERV_UNAV);
			//fill Code
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			break;
		default:
			//"Date", "Server"
			this->code.SetCode(INT_SERV_ERR);
			//fill Code
			AddHeader(this->code, Date());
			AddHeader(this->code, Server(serv));
			break;
		}
	}
}


ostream& operator<<(std::ostream& os, const HttpResponse& r) {
	cout << r.code;
	if (r.fd) {
		//вывод содержимого файла
	}
	return os;
}