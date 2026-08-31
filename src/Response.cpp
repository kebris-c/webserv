/* **************************************************************************
 * Response.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

#include "Response.hpp"
#include "Utils.hpp"

Response::Response() : _status(200) {}
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

void	Response::setBodyFromFile(const std::string &path)
{
	/* TODO(kmarrero): open/read file; set Content-Type from extension */
	(void)path;
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
