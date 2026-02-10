#include "CGIHandler.hpp"
#include "Utils.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <fstream>

CGIHandler::CGIHandler(const HTTPRequest& request, const std::string& scriptPath)
    : _request(request), _scriptPath(scriptPath), _pid(-1), _running(false),
      _exitStatus(0), _bodyWritten(0), _stdinClosed(false) {
    _pipeIn[0] = _pipeIn[1] = -1;
    _pipeOut[0] = _pipeOut[1] = -1;
    _pipeErr[0] = _pipeErr[1] = -1;
}

CGIHandler::~CGIHandler() {
    closePipes();
}

void CGIHandler::setupPipes() {
    if (pipe(_pipeIn) == -1)
        throw std::runtime_error("Failed to create stdin pipe");
    if (pipe(_pipeOut) == -1) {
        close(_pipeIn[0]);
        close(_pipeIn[1]);
        throw std::runtime_error("Failed to create stdout pipe");
    }
    if (pipe(_pipeErr) == -1) {
        close(_pipeIn[0]);
        close(_pipeIn[1]);
        close(_pipeOut[0]);
        close(_pipeOut[1]);
        throw std::runtime_error("Failed to create stderr pipe");
    }
}

void CGIHandler::closePipes() {
    if (_pipeIn[0] != -1) close(_pipeIn[0]);
    if (_pipeIn[1] != -1) close(_pipeIn[1]);
    if (_pipeOut[0] != -1) close(_pipeOut[0]);
    if (_pipeOut[1] != -1) close(_pipeOut[1]);
    if (_pipeErr[0] != -1) close(_pipeErr[0]);
    if (_pipeErr[1] != -1) close(_pipeErr[1]);
}

void CGIHandler::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("Failed to get file descriptor flags");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set non-blocking mode");
}

char** CGIHandler::buildEnvironment() {
    std::vector<std::string> envStrings;
    
    // CGI/1.1 required environment variables
    envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envStrings.push_back("SERVER_PROTOCOL=" + _request.getVersion());
    envStrings.push_back("SERVER_SOFTWARE=WebServ/1.0");
    envStrings.push_back("REQUEST_METHOD=" + _request.getMethod());
    
    // Parse URI for SCRIPT_NAME and QUERY_STRING
    std::string uri = _request.getURI();
    size_t queryPos = uri.find('?');
    std::string scriptName = (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;
    std::string queryString = (queryPos != std::string::npos) ? uri.substr(queryPos + 1) : "";
    
    envStrings.push_back("SCRIPT_NAME=" + scriptName);
    envStrings.push_back("SCRIPT_FILENAME=" + _scriptPath);
    envStrings.push_back("QUERY_STRING=" + queryString);
    envStrings.push_back("PATH_INFO=" + scriptName);
    envStrings.push_back("PATH_TRANSLATED=" + _scriptPath);
    
    // Content-related
    std::string contentType = _request.getHeader("Content-Type");
    if (!contentType.empty())
        envStrings.push_back("CONTENT_TYPE=" + contentType);
    
    std::string contentLength = _request.getHeader("Content-Length");
    if (!contentLength.empty())
        envStrings.push_back("CONTENT_LENGTH=" + contentLength);
    else if (_request.isChunked())
        envStrings.push_back("CONTENT_LENGTH=" + Utils::intToString(_request.getUnchunkedBody().length()));
    
    // HTTP headers as HTTP_*
    const std::map<std::string, std::string>& headers = _request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string key = "HTTP_" + Utils::toUpper(it->first);
        // Replace hyphens with underscores
        for (size_t i = 0; i < key.length(); i++) {
            if (key[i] == '-')
                key[i] = '_';
        }
        envStrings.push_back(key + "=" + it->second);
    }
    
    // Server info (hardcoded for simplicity)
    envStrings.push_back("SERVER_NAME=localhost");
    envStrings.push_back("SERVER_PORT=8080");
    envStrings.push_back("REMOTE_ADDR=127.0.0.1");
    
    // Allocate array
    char** env = new char*[envStrings.size() + 1];
    for (size_t i = 0; i < envStrings.size(); i++) {
        env[i] = new char[envStrings[i].length() + 1];
        std::strcpy(env[i], envStrings[i].c_str());
    }
    env[envStrings.size()] = NULL;
    
    return env;
}

void CGIHandler::freeEnvironment(char** env) {
    if (!env)
        return;
    for (int i = 0; env[i] != NULL; i++)
        delete[] env[i];
    delete[] env;
}

std::string CGIHandler::resolvePath() {
    // Simple path resolution - in a real implementation, this would be more sophisticated
    return _scriptPath;
}

std::string CGIHandler::getInterpreter(const std::string& scriptPath) {
    // Read first line to check for shebang
    std::ifstream file(scriptPath.c_str());
    if (!file.is_open())
        return "";
    
    std::string firstLine;
    std::getline(file, firstLine);
    file.close();
    
    // Check for shebang
    if (firstLine.length() >= 2 && firstLine[0] == '#' && firstLine[1] == '!') {
        // Extract interpreter path
        std::string interpreter = firstLine.substr(2);
        
        // Trim whitespace
        size_t start = 0;
        while (start < interpreter.length() && std::isspace(interpreter[start]))
            start++;
        interpreter = interpreter.substr(start);
        
        // Handle "#!/usr/bin/env python3" format
        if (interpreter.find("/usr/bin/env ") == 0) {
            std::string prog = interpreter.substr(13);
            // Trim whitespace
            start = 0;
            while (start < prog.length() && std::isspace(prog[start]))
                start++;
            size_t end = prog.length();
            while (end > start && (std::isspace(prog[end - 1]) || prog[end - 1] == '\r'))
                end--;
            return prog.substr(start, end - start);
        }
        
        // Remove trailing whitespace/carriage return
        size_t end = interpreter.length();
        while (end > 0 && (std::isspace(interpreter[end - 1]) || interpreter[end - 1] == '\r'))
            end--;
        return interpreter.substr(0, end);
    }
    
    return "";
}

bool CGIHandler::execute() {
    try {
        // Setup pipes
        setupPipes();
        
        // Set parent's pipe ends to non-blocking
        setNonBlocking(_pipeIn[1]);  // Write to CGI stdin
        setNonBlocking(_pipeOut[0]); // Read from CGI stdout
        setNonBlocking(_pipeErr[0]); // Read from CGI stderr
        
        // Get the body (unchunked if needed)
        _requestBody = _request.getUnchunkedBody();
        
        // Fork
        _pid = fork();
        
        if (_pid == -1) {
            closePipes();
            return false;
        }
        
        if (_pid == 0) {
            // Child process
            
            // Redirect stdin, stdout, stderr
            if (dup2(_pipeIn[0], STDIN_FILENO) == -1)
                exit(1);
            if (dup2(_pipeOut[1], STDOUT_FILENO) == -1)
                exit(1);
            if (dup2(_pipeErr[1], STDERR_FILENO) == -1)
                exit(1);
            
            // Close all pipe file descriptors
            close(_pipeIn[0]);
            close(_pipeIn[1]);
            close(_pipeOut[0]);
            close(_pipeOut[1]);
            close(_pipeErr[0]);
            close(_pipeErr[1]);
            
            // Build environment
            char** env = buildEnvironment();
            
            // Prepare arguments
            std::string resolvedPath = resolvePath();
            std::string interpreter = getInterpreter(resolvedPath);
            
            if (!interpreter.empty()) {
                // Execute using interpreter
                char* argv[3];
                argv[0] = const_cast<char*>(interpreter.c_str());
                argv[1] = const_cast<char*>(resolvedPath.c_str());
                argv[2] = NULL;
                
                // If interpreter is absolute path, use it directly
                if (interpreter[0] == '/') {
                    execve(argv[0], argv, env);
                } else {
                    // Try to find interpreter in common paths
                    std::string interpreterPaths[] = {
                        "/usr/bin/" + interpreter,
                        "/bin/" + interpreter,
                        "/usr/local/bin/" + interpreter
                    };
                    
                    for (int i = 0; i < 3; i++) {
                        argv[0] = const_cast<char*>(interpreterPaths[i].c_str());
                        execve(argv[0], argv, env);
                    }
                }
            } else {
                // Try to execute directly
                char* argv[2];
                argv[0] = const_cast<char*>(resolvedPath.c_str());
                argv[1] = NULL;
                execve(argv[0], argv, env);
            }
            
            // If execve fails
            exit(1);
        }
        
        // Parent process
        
        // Close child's pipe ends
        close(_pipeIn[0]);
        _pipeIn[0] = -1;
        close(_pipeOut[1]);
        _pipeOut[1] = -1;
        close(_pipeErr[1]);
        _pipeErr[1] = -1;
        
        _running = true;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "CGI execution error: " << e.what() << std::endl;
        return false;
    }
}

bool CGIHandler::writeToStdin() {
    if (_stdinClosed || _pipeIn[1] == -1)
        return true;
    
    if (_bodyWritten >= _requestBody.length()) {
        close(_pipeIn[1]);
        _pipeIn[1] = -1;
        _stdinClosed = true;
        return true;
    }
    
    ssize_t written = write(_pipeIn[1], 
                           _requestBody.c_str() + _bodyWritten,
                           _requestBody.length() - _bodyWritten);
    
    if (written == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return false; // Would block, try again later
        
        // Error occurred
        close(_pipeIn[1]);
        _pipeIn[1] = -1;
        _stdinClosed = true;
        return true;
    }
    
    _bodyWritten += written;
    
    if (_bodyWritten >= _requestBody.length()) {
        close(_pipeIn[1]);
        _pipeIn[1] = -1;
        _stdinClosed = true;
    }
    
    return true;
}

bool CGIHandler::readFromStdout() {
    if (_pipeOut[0] == -1)
        return true;
    
    char buffer[4096];
    ssize_t bytesRead = read(_pipeOut[0], buffer, sizeof(buffer));
    
    if (bytesRead == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return false; // Would block, try again later
        
        // Error occurred
        close(_pipeOut[0]);
        _pipeOut[0] = -1;
        return true;
    }
    
    if (bytesRead == 0) {
        // EOF
        close(_pipeOut[0]);
        _pipeOut[0] = -1;
        return true;
    }
    
    _responseData.append(buffer, bytesRead);
    return true;
}

bool CGIHandler::isDone() const {
    // Check if child process has exited
    if (_running && _pid != -1) {
        int status;
        pid_t result = waitpid(_pid, &status, WNOHANG);
        
        if (result == _pid) {
            CGIHandler* self = const_cast<CGIHandler*>(this);
            self->_running = false;
            if (WIFEXITED(status))
                self->_exitStatus = WEXITSTATUS(status);
        }
    }
    
    // Done when child exited, stdin closed, and stdout read
    return !_running && _stdinClosed && (_pipeOut[0] == -1);
}

int CGIHandler::getStdinFd() const {
    return _pipeIn[1];
}

int CGIHandler::getStdoutFd() const {
    return _pipeOut[0];
}

HTTPResponse CGIHandler::parseCGIOutput(const std::string& output) {
    HTTPResponse response;
    
    // Parse CGI output
    // CGI output format:
    // Headers\r\n\r\nBody
    
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // No proper CGI headers, treat all as body
        response.setStatus(200, "OK");
        response.setBody(output);
        return response;
    }
    
    std::string headerSection = output.substr(0, headerEnd);
    std::string body = output.substr(headerEnd + 4);
    
    // Parse headers
    std::vector<std::string> lines = Utils::split(headerSection, '\n');
    bool hasStatus = false;
    
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = Utils::trim(lines[i]);
        if (line.empty())
            continue;
        
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;
        
        std::string key = Utils::trim(line.substr(0, colonPos));
        std::string value = Utils::trim(line.substr(colonPos + 1));
        
        if (Utils::toLower(key) == "status") {
            // Parse status: "200 OK"
            size_t spacePos = value.find(' ');
            if (spacePos != std::string::npos) {
                int code = Utils::stringToInt(value.substr(0, spacePos));
                std::string message = value.substr(spacePos + 1);
                response.setStatus(code, message);
                hasStatus = true;
            }
        } else {
            response.setHeader(key, value);
        }
    }
    
    if (!hasStatus)
        response.setStatus(200, "OK");
    
    response.setBody(body);
    
    return response;
}

HTTPResponse CGIHandler::getResponse() {
    return parseCGIOutput(_responseData);
}

bool CGIHandler::isRunning() const {
    return _running;
}

int CGIHandler::getExitStatus() const {
    return _exitStatus;
}
