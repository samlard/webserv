#!/bin/bash
echo "Content-Type: text/html"
echo ""
echo "<html><body>"
echo "<h1>Hello from CGI!</h1>"
echo "<p>Request Method: $REQUEST_METHOD</p>"
echo "<p>Query String: $QUERY_STRING</p>"
echo "<p>Server Protocol: $SERVER_PROTOCOL</p>"
echo "</body></html>"
