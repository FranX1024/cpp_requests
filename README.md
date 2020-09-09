# C++ HTTP library

## Introduction

requests.cpp is a C++ library that implements HTTP client.

## Basics

requests.cpp uses `http` namespace and all functions and structs are inside it.
In most cases, you'll want to create an `http::request` object, fill it with information you want to submit (method, url, headers, body).
Then you'd create an `http::response` object, which will store the response from web server.
Finally, you'll call `http::open` which accepts an `http::request` as argument and returns `http::response`.

### Example of an http client sending a GET request

```c++
#include <stdio.h>
#include "requests.cpp"

using namespace std;

int main() {
	http::response resp = http::open((http::request){
		.method = "GET",
		.url = "http://jsonplaceholder.typicode.com/todos/42"
	});
	printf("RESPONSE: %s\n", resp.body.c_str());
	return 0;
}
```

### setting headers and body

```c++
http::request req;
req.url = "<url>";
req.method = "<GET, POST, ...>";
req.header["Header-Name"] = "Header-Content";
req.body = "<Request body>";
```
