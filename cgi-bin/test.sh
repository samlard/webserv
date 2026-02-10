#!/bin/bash

echo "Content-Type: text/html"
echo "Status: 200 OK"
echo ""
echo "<html><body>"
echo "<h1>Hello from Bash CGI!</h1>"
echo "<p>REQUEST_METHOD: $REQUEST_METHOD</p>"
echo "<p>QUERY_STRING: $QUERY_STRING</p>"

if [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
    echo "<h2>POST Data:</h2>"
    echo "<pre>"
    cat
    echo "</pre>"
fi

echo "</body></html>"
