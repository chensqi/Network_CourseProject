#use: python test.py <port number> <timeout (cache gets cleared in client after timeout) in secs>
import unittest
import telnetlib
import time
import thread
import threading
import subprocess
import sys
import os


os.system("pkill -f webproxy")
subprocess.Popen(["./webproxy",sys.argv[1], sys.argv[2]])
time.sleep(2)

class TestProxy(unittest.TestCase):
       
    def connect(self):
        self.tn = telnetlib.Telnet("localhost", self.port)

    def exec_time(self, url, keep=0):
        print "called", url, keep
        if keep!=2:
            self.connect()
        start = time.time()
        if keep==1:
            self.tn.write("GET "+ url+ " HTTP/1.1\nConnection: keep-alive")    
        else:
            self.tn.write("GET "+ url+ " HTTP/1.0")
        self.tn.read_all()
        exectime = (time.time() - start)
        return exectime
    def test_1from_server(self):
        url = 'http://www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html'
        self.assertGreater(self.exec_time(url), 5)

    def test_2from_proxy(self):
        url = 'http://www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html'
        self.assertLess(self.exec_time(url), 5)    

    def test_3clear_proxy(self):
        # print "sleeping for ", float(self.proxyTimeout), "s to clear cache"
        time.sleep(float(self.proxyTimeout))
        url = 'http://www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html'
        self.assertGreater(self.exec_time(url), 5)

    def test_4prefetch(self):
        time.sleep(float(self.proxyTimeout))
        url = 'http://www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html'
        self.exec_time(url)
        url = "http://www-personal.umich.edu/~kevinand/first.html"
        self.assertLess(self.exec_time(url), 5) 
        url = "http://www-personal.umich.edu/~zechen/first.html"
        self.assertLess(self.exec_time(url), 5) 

    def test_5multithread(self):
        try:
                t1=threading.Thread(target=self.exec_time, args=("http://www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html",))
                t2=threading.Thread(target=self.exec_time, args=("http://www.wix.com/",))
                t1.start()
                t2.start()
                allthreads = []
                allthreads.append(t1)
                allthreads.append(t2)
                for t in allthreads:
                        t.join()
        except:
            print "Error: unable to start thread"
            assert(0)

    def test_6keepalive(self):
        url = 'http://www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html'
        self.exec_time(url,1)
        self.assertLess(self.exec_time(url,2),5)

if __name__ == '__main__':
    TestProxy.proxyTimeout = sys.argv.pop()
    TestProxy.port = sys.argv.pop()
    unittest.main()


