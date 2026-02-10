#!/usr/bin/env python3
import os
import datetime

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI Test</h1>")
print("<p>This is a CGI script executed via fork() + execve()</p>")
print("<p>Request Method: " + os.environ.get('REQUEST_METHOD', 'N/A') + "</p>")
print("<p>Query String: " + os.environ.get('QUERY_STRING', 'N/A') + "</p>")
print("<p>Current Time: " + str(datetime.datetime.now()) + "</p>")
print("</body></html>")
