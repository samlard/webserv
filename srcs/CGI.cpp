#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"
#include <sys/stat.h>
#include <poll.h>
#include <ctime>



static std::string getQueryString(const std::string& uri)
{
    size_t pos = uri.find('?');
    if (pos == std::string::npos)
        return "";

    size_t end = uri.find('#', pos + 1);
    if (end == std::string::npos)
        return uri.substr(pos + 1);
    return uri.substr(pos + 1, end - (pos + 1));
}

static std::string normalizeLocationPath(const std::string& path)
{
    if (path.empty())
        return "/";

    if (path.size() > 1 && path[path.size() - 1] == '/')
        return path.substr(0, path.size() - 1);

    return path;
}

Response Server::executeCgi(const Request& req, const std::string& scriptPath, Location* loc)
{
    Response res;
    int outPipe[2];
    int inPipe[2];
    struct stat st;
    const int CGI_TIMEOUT_MS = 10000;

    if (stat(scriptPath.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
    {
        std::cerr << "[CGI] script not found or not a regular file: " << scriptPath << std::endl;
        res.statusCode = 404;
        res.body = "CGI script not found";
        return res;
    }

    if (access(scriptPath.c_str(), R_OK) != 0)
    {
        std::cerr << "[CGI] script is not readable: " << scriptPath << std::endl;
        res.statusCode = 403;
        res.body = "CGI script is not readable";
        return res;
    }

    if (!loc->cgi_path.empty() && access(loc->cgi_path.c_str(), X_OK) != 0)
    {
        std::cerr << "[CGI] interpreter is not executable: " << loc->cgi_path << std::endl;
        res.statusCode = 500;
        res.body = "CGI interpreter is not executable";
        return res;
    }

    if (loc->cgi_path.empty() && access(scriptPath.c_str(), X_OK) != 0)
    {
        std::cerr << "[CGI] script is not executable and no cgi_path provided: " << scriptPath << std::endl;
        res.statusCode = 403;
        res.body = "CGI script is not executable";
        return res;
    }

    std::cout << "[CGI] executing script=" << scriptPath
              << " method=" << req.method
              << " uri=" << req.uri
              << " interpreter=" << (loc->cgi_path.empty() ? "<shebang>" : loc->cgi_path)
              << std::endl;

    if (pipe(outPipe) < 0)
    {
        res.statusCode = 500;
        res.body = "CGI output pipe error";
        return res;
    }

    if (pipe(inPipe) < 0)
    {
        close(outPipe[0]);
        close(outPipe[1]);
        res.statusCode = 500;
        res.body = "CGI input pipe error";
        return res;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(outPipe[0]);
        close(outPipe[1]);
        close(inPipe[0]);
        close(inPipe[1]);
        res.statusCode = 500;
        res.body = "CGI fork error";
        return res;
    }

    if (pid == 0)
    {
        // chdir to script directory for relative path file access
        std::string scriptName = scriptPath;
        std::string dir = scriptPath;
        size_t lastSlash = dir.rfind('/');
        if (lastSlash != std::string::npos)
        {
            dir = dir.substr(0, lastSlash);
            scriptName = scriptPath.substr(lastSlash + 1);
            if (!dir.empty())
                chdir(dir.c_str());
        }

        dup2(outPipe[1], STDOUT_FILENO);
        dup2(outPipe[1], STDERR_FILENO);
        dup2(inPipe[0], STDIN_FILENO);

        close(outPipe[0]);
        close(outPipe[1]);
        close(inPipe[0]);
        close(inPipe[1]);

        std::string requestMethod = "REQUEST_METHOD=" + req.method;
        std::string queryString = "QUERY_STRING=" + getQueryString(req.uri);
        std::string serverProtocol = "SERVER_PROTOCOL=" + req.version;
        std::stringstream clss;
        clss << req.body.size();
        std::string contentLength = "CONTENT_LENGTH=" + clss.str();
        std::string contentTypeValue = "text/plain";
        std::map<std::string, std::string>::const_iterator contentTypeIt = req.headers.find("Content-Type");
        if (contentTypeIt != req.headers.end() && !contentTypeIt->second.empty())
            contentTypeValue = contentTypeIt->second;
        std::string contentType = "CONTENT_TYPE=" + contentTypeValue;
        std::string scriptFilename = "SCRIPT_FILENAME=" + scriptPath;

        // Get PATH from parent environment so shebangs with /usr/bin/env work
        std::string pathEnv = "PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin";
        const char* parentPath = getenv("PATH");
        if (parentPath)
            pathEnv = std::string("PATH=") + parentPath;

        char* envp[8];
        envp[0] = const_cast<char*>(requestMethod.c_str());
        envp[1] = const_cast<char*>(queryString.c_str());
        envp[2] = const_cast<char*>(serverProtocol.c_str());
        envp[3] = const_cast<char*>(contentLength.c_str());
        envp[4] = const_cast<char*>(contentType.c_str());
        envp[5] = const_cast<char*>(scriptFilename.c_str());
        envp[6] = const_cast<char*>(pathEnv.c_str());
        envp[7] = NULL;

        // Use script filename (not full path) since we chdir'd to script dir
        std::string dotSlashScript = "./" + scriptName;

        if (!loc->cgi_path.empty())
        {
            char* argv[3];
            argv[0] = const_cast<char*>(loc->cgi_path.c_str());
            argv[1] = const_cast<char*>(dotSlashScript.c_str());
            argv[2] = NULL;
            execve(argv[0], argv, envp);
        }
        else
        {
            char* argv[2];
            argv[0] = const_cast<char*>(dotSlashScript.c_str());
            argv[1] = NULL;
            execve(argv[0], argv, envp);
        }

        perror("execve failed");
        _exit(1);
    }

    // Parent process
    close(outPipe[1]);
    close(inPipe[0]);

    // Set pipes non-blocking for poll-based I/O
    fcntl(inPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(outPipe[0], F_SETFL, O_NONBLOCK);

    time_t startTime = time(NULL);

    // Write request body to CGI stdin using poll()
    if (!req.body.empty())
    {
        size_t written = 0;
        while (written < req.body.size())
        {
            if (time(NULL) - startTime > CGI_TIMEOUT_MS / 1000)
            {
                kill(pid, SIGKILL);
                waitpid(pid, NULL, 0);
                close(inPipe[1]);
                close(outPipe[0]);
                res.statusCode = 504;
                res.body = "CGI timeout";
                return res;
            }

            struct pollfd pfd;
            pfd.fd = inPipe[1];
            pfd.events = POLLOUT;
            pfd.revents = 0;

            int pret = poll(&pfd, 1, 1000);
            if (pret <= 0)
                break;
            if (!(pfd.revents & POLLOUT))
                break;

            ssize_t n = write(inPipe[1], req.body.c_str() + written, req.body.size() - written);
            if (n <= 0)
                break;
            written += static_cast<size_t>(n);
        }
    }
    close(inPipe[1]);

    // Read CGI output using poll()
    char buffer[4096];
    std::string output;

    while (true)
    {
        if (time(NULL) - startTime > CGI_TIMEOUT_MS / 1000)
        {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            close(outPipe[0]);
            res.statusCode = 504;
            res.body = "CGI timeout";
            return res;
        }

        struct pollfd pfd;
        pfd.fd = outPipe[0];
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pret = poll(&pfd, 1, 1000);
        if (pret < 0)
            break;
        if (pret == 0)
            continue;
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            // Try one last read before breaking
            ssize_t bytes = read(outPipe[0], buffer, sizeof(buffer));
            if (bytes > 0)
                output.append(buffer, bytes);
            break;
        }
        if (pfd.revents & POLLIN)
        {
            ssize_t bytes = read(outPipe[0], buffer, sizeof(buffer));
            if (bytes <= 0)
                break;
            output.append(buffer, bytes);
        }
    }

    close(outPipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if ((!WIFEXITED(status) || WEXITSTATUS(status) != 0) && output.empty())
    {
        res.statusCode = 500;
        res.body = "CGI execution failed";
        return res;
    }

    size_t headerEnd = output.find("\r\n\r\n");
    size_t headerDelimiterLen = 4;
    if (headerEnd == std::string::npos)
    {
        headerEnd = output.find("\n\n");
        headerDelimiterLen = 2;
    }

    if (headerEnd == std::string::npos)
    {
        std::cerr << "[CGI] invalid response, missing header/body separator. Raw output: "
                  << output.substr(0, 200) << std::endl;
        res.statusCode = 500;
        res.body = "Invalid CGI response";
        return res;
    }

    std::string headerPart = output.substr(0, headerEnd);
    std::string bodyPart = output.substr(headerEnd + headerDelimiterLen);

    std::istringstream headerStream(headerPart);
    std::string line;

    while (std::getline(headerStream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        size_t sep = line.find(':');
        if (sep != std::string::npos)
        {
            std::string key = line.substr(0, sep);
            std::string value = line.substr(sep + 1);
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                value.erase(0, 1);
            res.headers[key] = value;
        }
    }

    if (res.headers.find("Status") != res.headers.end())
    {
        std::istringstream ss(res.headers["Status"]);
        ss >> res.statusCode;
        res.headers.erase("Status");
    }
    else
    {
        res.statusCode = 200;
    }

    res.body = bodyPart;

    std::stringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();

    return res;
}

Response Server::handleGet(const Request& req, const ServerConfig& config, Location* loc)
{
    Response res;

    std::string uri = stripUriSuffix(req.uri);
    const std::string root = !loc->root.empty() ? loc->root : config.root;
    const std::string index = !loc->index.empty() ? loc->index : config.index;

    std::cout << "Requested URI: " << uri << std::endl;

    // If URI is root, serve the configured index file.
    if (uri == "/" && !index.empty()) {
        uri = "/" + index;
    }

    std::string path = buildPathFromLocation(uri, root, loc);

    std::cout << "Requested I: " << uri << std::endl;
    std::cout << "Resolved path: " << path << std::endl;

    if (isCgiRequest(uri, loc))
    {
        std::cout << "[CGI] GET detected as CGI for uri=" << uri << std::endl;
        return executeCgi(req, path, loc);
    }

    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        // If URI doesn't end with '/', redirect to the canonical directory URL
        if (uri.empty() || uri[uri.size() - 1] != '/')
        {
            res.statusCode = 301;
            res.headers["Location"] = uri + "/";
            res.body = "";
            return res;
        }

        std::string indexPath = path;
        if (!index.empty())
        {
            if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
                indexPath += "/";
            indexPath += index;

            std::ifstream indexFile(indexPath.c_str());
            if (indexFile.is_open())
            {
                std::stringstream indexBuffer;
                indexBuffer << indexFile.rdbuf();
                res.statusCode = 200;
                res.body = indexBuffer.str();
                return res;
            }
        }

        if (loc->autoindex)
        {
            res.statusCode = 200;
            res.body = generateAutoindexBody(uri, path);
            return res;
        }

        res.statusCode = 403;
        res.body = "403 Forbidden";
        return res;
    }

    std::ifstream file(path.c_str());

    if (!file.is_open())
    {
        res.statusCode = 404;
        res.body = "404 Not Found";
        return res;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    res.statusCode = 200;
    res.body = buffer.str();

    std::stringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    res.headers["Content-Type"] = getMimeType(path);

    return res;
}

Response Server::handlePost(const Request& req, const ServerConfig& config, Location* loc)
{
    Response res;
    std::string uri = stripUriSuffix(req.uri);
    std::string root = !loc->root.empty() ? loc->root : config.root;
    std::string path = buildPathFromLocation(uri, root, loc);

    if (isCgiRequest(uri, loc))
    {
        std::cout << "[CGI] POST detected as CGI for uri=" << uri << std::endl;
        return executeCgi(req, path, loc);
    }

    if (!loc->upload_path.empty())
    {
        static unsigned long uploadCounter = 0;
        uploadCounter++;

        std::stringstream filePath;
        filePath << loc->upload_path;
        if (loc->upload_path[loc->upload_path.size() - 1] != '/')
            filePath << "/";
        filePath << "upload_" << time(NULL) << "_" << uploadCounter;

        std::ofstream out(filePath.str().c_str(), std::ios::binary);
        if (!out.is_open())
        {
            res.statusCode = 500;
            res.body = "Upload failed";
            return res;
        }

        out << req.body;
        out.close();

        res.statusCode = 201;
        res.body = "Created";
        return res;
    }

    res.statusCode = 200;
    res.body = "POST received";

    return res;
}

Response Server::handleDelete(const Request& req, const ServerConfig& config, Location* loc)
{
    Response res;
    std::string uri = stripUriSuffix(req.uri);
    std::string root = !loc->root.empty() ? loc->root : config.root;
    std::string path = buildPathFromLocation(uri, root, loc);

    if (remove(path.c_str()) == 0)
    {
        res.statusCode = 200;
        res.body = "File deleted";
    }
    else
    {
        res.statusCode = 404;
        res.body = "File not found";
    }

    return res;
}

Location* Server::matchLocation(const ServerConfig& config, const std::string& uri)
{
    Location* bestMatch = NULL;
    size_t bestLen = 0;
    // std::cout << uri << std::endl;
    std::string cleanUri = stripUriSuffix(uri);

    if (cleanUri.empty())
        cleanUri = "/";

    for (size_t i = 0; i < config.locations.size(); i++)
    {
        const std::string locPath = normalizeLocationPath(config.locations[i].path);

        if (locPath.empty())
            continue;

        bool isBoundaryMatch = false;
        if (locPath == "/")
            isBoundaryMatch = true;
        else if (cleanUri == locPath)
            isBoundaryMatch = true;
        else if (cleanUri.find(locPath + "/") == 0)
            isBoundaryMatch = true;

        if (!isBoundaryMatch)
            continue;

        if (locPath.size() > bestLen)
        {
            bestLen = locPath.size();
            bestMatch = (Location*)&config.locations[i];
        }
    }

    return bestMatch;
}

bool Server::isCgiRequest(const std::string& path, Location* loc)
{
    if (!loc)
        return false;

    if (loc->cgi_extensions.empty())
        return false;

    std::string cleanPath = stripUriSuffix(path);

    if (!cleanPath.empty() && cleanPath[cleanPath.size() - 1] == '/')
        cleanPath.erase(cleanPath.size() - 1);

    // Check if path ends with any of the configured CGI extensions
    for (size_t i = 0; i < loc->cgi_extensions.size(); ++i) {
        const std::string& ext = loc->cgi_extensions[i];
        if (cleanPath.size() >= ext.size()) {
            if (cleanPath.substr(cleanPath.size() - ext.size()) == ext) {
                std::cout << "[CGI] isCgiRequest path=" << cleanPath
                          << " extension=" << ext
                          << " matched=yes" << std::endl;
                return true;
            }
        }
    }

    std::cout << "[CGI] isCgiRequest path=" << cleanPath
              << " matched=no" << std::endl;
    return false;
}