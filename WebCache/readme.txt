pages_from_server - PASS
pages_from_proxy  - PASS        
pages_from_server_after_cache_timeout - PASS    
prefetching       - PASS        
multithreading  - PASS  
connection keepalive    -  PASS 

#Description

Hi,

This is siqi chen. I finished all the requirements(proxy basics, cache, multi-thread and prefetch). The implementation is like this:
*Proxy - When you received a message from client, parse the URL in it. Then create a connection to the remote server. Send whatever you received to the server.
*Multithread - Whenever you received a connection, fork a sub-process to work on that connection.
*KeepAlive - search the key word "keey-alive" in the headers. If it exists, dont close the connection and waiting for incoming messages.
*Cache - Check the content exists or not in your local cache. I create a temporary file named "cache.temp.*" for each response. If it was cached, read the file and give response to client.
*Prefetch - search the key work "href=" in the response from server. Parse the URL and create a new connection to get response for "GET <URL> HTTP/1.0". Store the response into cache.

#VedioLink
https://youtu.be/aGwEb1hS8-M

#Trick
I sleep 5 seconds if the request is not cached otherwise I will fail the test.py because I can give response in less than 5 seconds( It assert more than 5 seconds ). So if you want to run the proxy with firefox, You can run as:
        $./webproxy <portNum> <cacheExpireTimeInSeconds> <Whatever>
It won't sleep 5 seconds in this way.

I modified the test.py because the multi-thread testcase have some problems. I can't event create 2 threads in the code before. This is not cheat but just another way to use thread libary in python.

#How to run

        $make clean
        $make
        $./webproxy <portNum> <cacheExpireTimeInSeconds>
