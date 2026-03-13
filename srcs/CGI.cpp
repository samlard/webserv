#include "../includes/Server.hpp"
#include "../includes/Utils.hpp" 
#include <sys/stat.h> 



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

Response Server::executeCgi(const Request& req, const std::string& scriptPath, Location* loc)
{
    Response res;
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        res.statusCode = 500;
        res.body = "CGI pipe error";
        return res;
    }
    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        res.statusCode = 500;
        res.body = "CGI fork error";
        return res;
    }
    if (pid == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        std::string requestMethod = "REQUEST_METHOD=" + req.method;
        std::string queryString = "QUERY_STRING=" + getQueryString(req.uri);
        std::string serverProtocol = "SERVER_PROTOCOL=" + req.version;

        char* envp[4];
        envp[0] = const_cast<char*>(requestMethod.c_str());
        envp[1] = const_cast<char*>(queryString.c_str());
        envp[2] = const_cast<char*>(serverProtocol.c_str());
        envp[3] = NULL;

        if (!loc->cgi_path.empty())
        {
            char* argv[3];
            argv[0] = const_cast<char*>(loc->cgi_path.c_str());
            argv[1] = const_cast<char*>(scriptPath.c_str());
            argv[2] = NULL;
            execve(argv[0], argv, envp);
        }
        else
        {
            char* argv[2];
            argv[0] = const_cast<char*>(scriptPath.c_str());
            argv[1] = NULL;
            execve(argv[0], argv, envp);
        }

        exit(1);
    }

    close(pipefd[1]);

    char buffer[4096];
    int bytes;
    std::string output;

    while ((bytes = read(pipefd[0], buffer, 4096)) > 0)
        output.append(buffer, bytes);

    close(pipefd[0]);
    int status = 0;
    waitpid(pid, &status, 0);

    if ((!WIFEXITED(status) || WEXITSTATUS(status) != 0) && output.empty())
    {
        res.statusCode = 500;
        res.body = "CGI execution failed";
        return res;
    }

    res.statusCode = 200;
    res.body = output;

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

    if (isCgiRequest(path, loc))
        return executeCgi(req, path, loc);

    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
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

    // C++98 workaround pour std::to_string
    std::stringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    res.headers["Content-Type"] = "text/html; charset=UTF-8";

    return res;
}

Response Server::handlePost(const Request& req, const ServerConfig& config, Location* loc)
{
    Response res;
    std::string uri = stripUriSuffix(req.uri);
    std::string root = !loc->root.empty() ? loc->root : config.root;
    std::string path = buildPathFromLocation(uri, root, loc);

    if (isCgiRequest(path, loc))
        return executeCgi(req, path, loc);

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
        const std::string& locPath = config.locations[i].path;

        if (locPath.empty())
            continue;

        if (cleanUri.find(locPath) != 0)
            continue;

        bool isBoundaryMatch = (locPath == "/" || cleanUri.size() == locPath.size() || cleanUri[locPath.size()] == '/');
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
    if (loc->cgi_extension.empty())
        return false;

    if (path.size() < loc->cgi_extension.size())
        return false;

    return path.substr(path.size() - loc->cgi_extension.size()) == loc->cgi_extension;
}
