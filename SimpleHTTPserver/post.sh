(echo $"POST /test.html HTTP/1.1\nHost: localhost\n\nFirstname=Siqi&SecondName=Chen&fullName=Siqi+Chen" ; sleep 10) | telnet 127.0.0.1 8097 

