#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

class CGIHandler {
public:
    CGIHandler(const HTTPRequest& request, const std::string& scriptPath);
    ~CGIHandler();

    // Execute CGI
    bool execute();
    
    // Non-blocking I/O
    bool writeToStdin();
    bool readFromStdout();
    bool isDone() const;
    
    // Get file descriptors for poll
    int getStdinFd() const;
    int getStdoutFd() const;
    
    // Get response
    HTTPResponse getResponse();
    
    // Process management
    bool isRunning() const;
    int getExitStatus() const;

private:
    const HTTPRequest& _request;
    std::string _scriptPath;
    
    // Pipes
    int _pipeIn[2];   // Parent writes to CGI stdin
    int _pipeOut[2];  // Parent reads from CGI stdout
    int _pipeErr[2];  // Parent reads from CGI stderr
    
    // Process
    pid_t _pid;
    bool _running;
    int _exitStatus;
    
    // I/O state
    std::string _requestBody;
    size_t _bodyWritten;
    std::string _responseData;
    bool _stdinClosed;
    
    // Helper methods
    void setupPipes();
    void closePipes();
    void setNonBlocking(int fd);
    char** buildEnvironment();
    void freeEnvironment(char** env);
    std::string resolvePath();
    std::string getInterpreter(const std::string& scriptPath);
    HTTPResponse parseCGIOutput(const std::string& output);
};

#endif
