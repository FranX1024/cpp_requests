#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h> 
#include <string.h>
#include <cstdlib>
#include <string>
#include <map>

int parseInt(std::string &s) {
	int x = 0, begin = 0;
	bool negative = false;
	if(s[begin] == '-') {
		negative = true;
		begin++;
	}
	for(int i = begin; i < s.length() && s[i] >= '0' && s[i] <= '9'; i++) {
		x = x * 10 + s[i] - '0';
	}
	if(negative) return -x;
	return x;
}

std::string toString(int x) {
	std::string s;
	if(x == 0) return "0";
	bool neg = false;
	if(x < 0) {
		x = ~x + 1;
		neg = true;
	}
	while(x) {
		char c = (x % 10) + '0';
		s = c + s;
		x /= 10;
	}
	if(neg) s = '-' + s;
	return s;
}

namespace http {
	
	struct response {
		std::string status_line;
		std::string body;
		std::map <std::string, std::string> header;
	};

	struct request {
		std::string method;
		std::string url;
		std::string body;
		std::map <std::string, std::string> header;
	};

	struct location {
		std::string host;
		std::string path;
		int port;
	};

	std::string raw_request(std::string host, int port, std::string req) {
		int sock = 0;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		// create buffer for storing response
		char buffer[1024] = {0};
		// if socket creation failed
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("ERROR: Cannot create socket!\n");
			std::exit(1);
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		// if host address wrong
		if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
			printf("ERROR: Invalid address!\n");
			std::exit(1);
		}
		// connect socket
		if(connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)	{
			printf("ERROR: Connection failed!\n");
			std::exit(1);
		}
		// send request
		send(sock, req.c_str(), strlen(req.c_str()), 0);
		// accept response
		std::string resp;
		// read first chunk (contains headers)
		recv(sock, buffer, 1024, 0);
		for(int i = 0; i < 1024 && buffer[i] != 0; i++) resp += buffer[i];
		// itterate through to get Content-Length header
		std::string line;
		std::string header = "Content-Length: ";
		int content_length = 0;
		int header_size = 0;
		for(int i = 0; i < 1024; i++) {
			if(buffer[i] == '\r' && buffer[i + 1] == '\n') {
				if(!line.length()) {
					header_size = i + 2;
					break;
				}
				if(line.length() > header.length()) {
					int c = 0;
					for(int j = 0; j < header.length(); j++) {
						if(header[j] == line[j]) {
							c++;
						} else break;
					}
					if(c == header.length()) {
						for(int j = header.length(); j < line.length(); j++) {
							content_length = content_length * 10 + line[j] - '0';
						}
					}
				}
				line = "";
				i++;
			}
			else line += buffer[i];
		}
		// reduce content_length by 1024, because of first chunk
		content_length += header_size - 1024;
		// continue reading while content_length > 0
		while(content_length > 0) {
			recv(sock, buffer, 1024, 0);
			content_length -= 1024;
			int am = 1024;
			if(content_length < 0) am = 1024 + content_length;
			for(int i = 0; i < am; i++) resp += buffer[i];
		}
		// return response
		return resp;
	}
	response parse_response(std::string body) {
		// create response struct
		response resp;
		// itterate through header lines
		int header_size = 0, line_number = 0;
		std::string line;
		for(int i = 0; i < body.length(); i++) {
			if(body[i] == '\r' && body[i + 1] == '\n') {
				if(!line.length()) {
					header_size = i + 2;
					break;
				}
				// first line is status line (should be HTTP/1.1 200 OK)
				if(line_number == 0) {
					resp.status_line = line;
				} else {
					// Put header in response headers
					int p = -1;
					for(int j = 0; j < line.length(); j++) {
						if(line[j] == ':') {
							p = j;
							break;
						}
					}
					if(p != -1) {
						std::string name, value;
						for(int j = 0; j < p; j++) name += line[j];
						for(int j = p + 2; j < line.length(); j++) value += line[j];
						resp.header[name] = value;
					}
				}
				// continue
				i++;
				line_number++;
				line = "";
			}
			else line += body[i];
		}
		for(int i = header_size; i < body.length(); i++) resp.body += body[i];
		return resp;
	}
	// get's ip from hostname (uses ip-api.com API)
	std::string get_ip(std::string hostname) {
		std::string host = "208.95.112.1";
		std::string req = "GET /csv/" + hostname + "?fields=8192 HTTP/1.1\r\nHost: ip-api.com\r\n\r\n";
		int port = 80;
		response resp = parse_response(raw_request(host, port, req));
		std::string ip = "";
		for(int i = 0; i < resp.body.length() && resp.body[i] != '\r' && resp.body[i] != '\n'; i++)
			ip += resp.body[i];
		return ip;
	}
	
	location parse_url(std::string url) {
		std::string protocol_name = "http://";
		std::string host, path;
		int port = 80; // default port
		// parsing
		int begin = 0, port_begin = url.length(), path_begin = url.length();
		// check if url begins with http://
		int matched = 0;
		for(int i = 0; i < url.length() && i < protocol_name.length(); i++)
			if(protocol_name[i] == url[i]) matched++;
		if(matched == protocol_name.length()) {
			begin = protocol_name.length();
		}
		// separate host from path
		for(int i = begin; i < url.length(); i++) {
			if(url[i] == ':') port_begin = i;
			if(url[i] == '/') {
				path_begin = i;
				break;
			}
		}
		// get host port and path
		for(int i = begin; i < port_begin && i < path_begin; i++) host += url[i];
		if(port_begin + 1 < path_begin) port = 0;
		for(int i = port_begin + 1; i < path_begin; i++) port = port * 10 + url[i] - '0';
		for(int i = path_begin + 1; i < url.length(); i++) path += url[i];
		// create location struct
		location loc;
		loc.host = host;
		loc.path = path;
		loc.port = port;
		return loc;
	}

	std::string request_builder(request req) {
		req.header["Content-Length"] = toString(req.body.length());
		location loc = parse_url(req.url);
		std::string reqstr;
		reqstr += req.method + " /" + loc.path + " HTTP/1.1\r\nHost: " + loc.host + "\r\n";
		for(std::map<std::string,std::string>::iterator it = req.header.begin(); it != req.header.end(); it++) {
			reqstr += it->first + ": " + it->second + "\r\n";
		}
		reqstr += "\r\n" + req.body;
		return reqstr;
	}

	response open(request req) {
		location loc = parse_url(req.url);
		std::string ip = get_ip(loc.host);
		std::string raw_req = request_builder(req);
		std::string raw_resp = raw_request(ip, loc.port, raw_req);
		response resp = parse_response(raw_resp);
		return resp;
	}
};
