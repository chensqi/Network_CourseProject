(echo $"GET /test.html HTTP/1.1\nHost: localhost\nConnection: Keep-alive\n\nGET /test.html HTTP/1.1\nHost: localhost\n\n" ; sleep 10) | telnet 127.0.0.1 8097 

