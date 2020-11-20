# proxy-server in C

Programming language: C/C++  
OS: Linux (Kali distro)  
Multi processes  

**1. GET/HEAD**
- GET request from web browser (FF or Chrome)  
    Test case:
        http://example.com/, 
        http://students.iitk.ac.in/gymkhana/, 
        http://phthiraptera.info/  
    Result: browser display the requested web sites  

- GET request from curl  
    Test case:
       $ curl -v --proxy localhost:8888 [http://www.hcmus.edu.vn](http://www.hcmus.edu.vn)  
    Result:
        browser display the requested web sites  
- HEAD request from curl  
    Test case:
        $ curl -v --proxy localhost:8888 -X HEAD [http://www.hcmus.edu.vn](http://www.hcmus.edu.vn)  
    Result:
        200 (OK) HTTP header  

2. Filltering URLs  
**input : $ ./<MSSV> 8888 fit.hcmus.edu**
- Suffix filtering  
    Test case:
        $ curl -v --proxy localhost:8888 [http://www.fit.hcmus.edu/~lqvu/](http://www.fit.hcmus.edu/~lqvu/) -> missing suffix '.vn'  
    Result: 
        403 (Forbidden) HTTP response. The proxy doesn't send the request to the web server  
- Prefix Filtering  
    Test case:
        $ curl -v --proxy localhost:8 888 [http://fit.hcmus.edu.vn/~lqvu/](http://fit.hcmus.edu.vn/~lqvu/) -> missing prefix 'www.'  
    Result:
        403 (Forbidden) HTTP reponse.  
- Non-filtering case  
    Test case:
        $ curl -v _–_ proxy localhost:8888 [http://www.fit.hcmus.edu.vn/~lqvu/](http://www.fit.hcmus.edu.vn/~lqvu/) -> has prefix and suffix  
    Result:
        301 (Moved Permanently) or 302 (Found) HTTP response from web server  

3. Simultaneously serve multiple requests  
    Test case:
        $./conctest.sh  
        (Script will open 4 terminals and each terminal will send a HTTP request to http://tuoitre.vn)  
    Result: 
        200 (OK) HTTP reponses  

4. Signal handling  
- SIGUSR2 (5 points)  
    Test case:
        $ pkill 1712695 --signal 12  
    Result: 
        Proxy shutdown  
- SIGINT  
    Test case: 
        Ctrl-C while executing  
    Result:
        Program ignore SIGINT  

5. Memory leak  
- Proxy side  
$ valgrind --leak-check=full --show-reachable=yes ./<MSSV> 8888
- Client side  
$ curl -v --proxy localhost:8888 [http://hcmus.edu.vn/](http://hcmus.edu.vn/)  
    Result:  
        LEAK SUMMARY:  
        definitely lost: 0 bytes in 0 blocks  
        indirectly lost: 0 bytes in 0 blocks  
        possibly lost: 0 bytes in 0 blocks  
        still reachable: 0 bytes in 0 blocks  
        suppressed: 0 bytes in 0 blocks  

6. Only allow GET and HEAD methods  
    Test case:
        $ curl -v --proxy localhost:8888 -X OPTIONS [http://hcmus.edu.vn/](http://hcmus.edu.vn/)  
    Result:
        405 (Method Not Allowed) or 501 (Not Implemented) HTTP response.  
        Proxy build custom response and send to client, doesn't send the request to web server.  

7. Non-HTTP request  
    **Bad HTTP request**  
    Test case:
        $ telnet localhost 8888  
        _<type ’abcdefg’ and enter>_  
    Result:
        Proxy ignores it and continue serving other requests  