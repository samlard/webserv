#!/usr/bin/env python3
import sys
import os

# Read POST data if any
content_length = os.environ.get('CONTENT_LENGTH', '0')
post_data = ''
if content_length and content_length != '0':
    post_data = sys.stdin.read(int(content_length))

# Output CGI headers
print("Content-Type: text/plain")
print("Status: 200 OK")
print()

# Output body
print("CGI Test Script")
print("=" * 40)
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'N/A')}")
print(f"CONTENT_LENGTH: {content_length}")
print(f"CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'N/A')}")
print(f"SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'N/A')}")
print()
if post_data:
    print(f"POST Data: {post_data}")
