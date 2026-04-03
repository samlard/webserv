#!/usr/bin/env python3
import os

content_length = os.environ.get("CONTENT_LENGTH", "0")
try:
	body_len = int(content_length)
except ValueError:
	body_len = 0

body = ""
if body_len > 0:
	body = os.read(0, body_len).decode("utf-8", "replace")

print("Content-Type: text/html")
print()
print("<!DOCTYPE html>")
print("<html><head><meta charset='UTF-8'><title>CGI POST Result</title></head><body>")
print("<h1>CGI WORKS</h1>")
print("<p>Method: %s</p>" % os.environ.get("REQUEST_METHOD", ""))
print("<p>Body:</p>")
print("<pre>%s</pre>" % body)
print("<a href='/'>Back</a>")
print("</body></html>")