/* **************************************************************************
 * Request.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

#include "Request.hpp"

Request::Request()
	: _state(REQ_LINE), _contentLength(0), _chunked(false), _errorCode(0)
{
}

Request::~Request() {}

void	Request::reset()
{
	_state = REQ_LINE;
	_method.clear();
	_target.clear();
	_query.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_contentLength = 0;
	_chunked = false;
	_errorCode = 0;
}

bool	Request::parse(std::string &buffer)
{
	/*
	 * TODO(kmarrero): incremental parser — see PSEUDOCODE in Request.hpp
	 * INVESTIGATE: CRLF rules, header folding (you can reject obsolete folding),
	 *              chunked coding, absolute-form targets from proxies (optional)
	 */
	(void)buffer;
	return (false);
}

RequestParseState	Request::state() const { return (_state); }
const std::string	&Request::method() const { return (_method); }
const std::string	&Request::target() const { return (_target); }
const std::string	&Request::query() const { return (_query); }
const std::string	&Request::version() const { return (_version); }
const std::map<std::string, std::string>	&Request::headers() const { return (_headers); }
const std::string	&Request::body() const { return (_body); }
int	Request::errorCode() const { return (_errorCode); }

std::string	Request::header(const std::string &name) const
{
	/* TODO(kmarrero): case-insensitive lookup */
	std::map<std::string, std::string>::const_iterator it = _headers.find(name);
	if (it == _headers.end())
		return ("");
	return (it->second);
}
