/* **************************************************************************
 * Response.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

#include "Response.hpp"
#include "Utils.hpp"

Response::Response()
	: _status(200)
{
	_extensionDispatcher["html"] =	"text/html";
	_extensionDispatcher["htm"] =	"text/html";
	_extensionDispatcher["css"] =	"text/css";
	_extensionDispatcher["js"] =	"application/javascript";
	_extensionDispatcher["png"] =	"image/png";
	_extensionDispatcher["jpg"] =	"image/jpg";
	_extensionDispatcher["pdf"] =	"application/pdf";
	_extensionDispatcher["txt"] =	"text/txt";
}
Response::~Response() {}

void	Response::setStatus(int code) { _status = code; }

void	Response::setHeader(const std::string &key, const std::string &value)
{
	_headers[key] = value;
}

void	Response::setBody(const std::string &body)
{
	_body = body;
	_headers["Content-Length"] = utils::toString(static_cast<int>(_body.size()));
}

std::string	Response::getExtension(const std::string& path)
{
	std::string::size_type	slash;
	std::string::size_type	dot;
	std::string				extension;

	slash = path.find_last_of('/');
	dot = path.find_last_of('.');
	if (dot != std::string::npos && dot > slash)
		extension = path.substr(dot + 1);
	return (extension);
}

void Response::setBodyFromFile(const std::string &path)
{
    std::string extension;
    std::string contentType;
    std::map<std::string, std::string>::const_iterator it;
    std::ifstream file(path.c_str(), std::ios::binary);
    std::istreambuf_iterator<char> begin(file);
    std::istreambuf_iterator<char> end;
    std::string body(begin, end);

    extension = getExtension(path);
    it = _extensionDispatcher.find(extension);
    if (it != _extensionDispatcher.end())
        contentType = it->second;
    else
        contentType = "application/octet-stream";

    setHeader("Content-Type", contentType);
    setBody(body);
}

std::string	Response::raw() const
{
	/*
	 * TODO(kmarrero): assemble:
	 *   HTTP/1.1 <code> <reason>\r\n
	 *   Header: value\r\n
	 *   ...
	 *   \r\n
	 *   body
	 */
	std::ostringstream	oss;
	oss << "HTTP/1.1 " << _status << " " << utils::statusReason(_status) << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";
	oss << "\r\n";
	oss << _body;
	return (oss.str());
}

Response	Response::makeError(int code, const std::string &pagePath)
{
	Response	r;
	r.setStatus(code);
	/* TODO(kmarrero): load pagePath if non-empty; else built-in HTML */
	(void)pagePath;
	r.setHeader("Content-Type", "text/html");
	r.setBody("<html><body><h1>" + utils::toString(code) + " "
			  + utils::statusReason(code) + "</h1></body></html>");
	return (r);
}
