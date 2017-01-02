How to make:
        $ make clean
        $ make

How to run:
        ./webServer

How to test server:
        Open http://localhost:8097/index.html from browser

What did I do:
        This is a HTTP server. Server will create a new thread each time there is a new connection, and give the response.
        Support HTTP/1.0 and HTTP/1.1 with GET and POST method.
        Keep alive when there is a Connection: Keep-alive.
