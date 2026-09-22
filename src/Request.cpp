/* **************************************************************************
 * Request.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

#include "Request.hpp"
#include "RequestUtils.hpp"

Request::Request()
	:_state(REQ_FEED), _contentLength(0), _chunked(false), _errorCode(0)
{}

Request::~Request()
{}

void	Request::reset()
{
	_state = REQ_FEED;
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

RequestState	Request::getCurrentState()
{
	return (this->_state);
}

void	Request::print(Context& ctx)
{
	std::cout << ctx.buffer << std::endl;
}

void	Request::setError(RequestState state, int errorCode)
{
	_state = state;
	_errorCode = errorCode;
}

void	Request::feed(const std::string& data, Context& ctx)
{
	ctx.buffer += data;
}

std::vector<std::string>	Request::split(const std::string& str, char delimiter)
{
	std::vector<std::string>	result;
	std::string::size_type		start = 0;
	std::string::size_type		pos;

	while ((pos = str.find(delimiter, start)) != std::string::npos)
	{
		result.push_back(str.substr(start, pos - start));
		start = pos + 1;
	}
	result.push_back(str.substr(start));
	return (result);
}

void	Request::parseTarget(std::string& target)
{
	std::string::size_type	pos;

	_query.clear();
	pos = target.find("?");
	if (pos != std::string::npos)
		_query = target.substr(pos + 1);
	else
		return ;
}

RequestState	Request::requestLine(Context& ctx)
{
	std::string::size_type		pos;
	std::vector<std::string>	line;
	std::string					requestLine;

	pos = ctx.buffer.find("\r\n");
	requestLine = ctx.buffer.substr(0, pos);
	line = split(requestLine, ' ');
	if (line.size() != 3)
		return (setError(REQ_ERROR, 400), REQ_ERROR);
	_method = line[0];
	if (!checkMethod(_method))
		return (setError(REQ_ERROR, 400), REQ_ERROR);
	_target = line[1];
	if (!checkTarget(_target))
		return (setError(REQ_ERROR, 400), REQ_ERROR);
	parseTarget(_target);
	_version = line[2];
	if (!checkVersion(_version))
		setError(REQ_ERROR, 400);
	_state = REQ_HEADERS;
	ctx.buffer.erase(0, pos + 2);
	return (REQ_HEADERS);
}

std::vector<std::string>	headerSplit(const std::string& str)
{
	std::vector<std::string>	result;
	std::string::size_type		start = 0;
	std::string::size_type		pos;

	while ((pos = str.find("\r\n", start)) != std::string::npos)
	{
		result.push_back(str.substr(start, pos - start));
		start += pos + 2;
	}
	if (start < str.size())
		result.push_back(str.substr(start));
	return (result);
}

RequestState	Request::requestHeader(Context& ctx)
{
	std::string	name;
	std::string	value;
	std::string	headerLines;
	std::vector<std::string> line;
	std::string::size_type	pos;
	std::string::size_type	colon;

	pos = ctx.buffer.find("\r\n\r\n");
	headerLines = ctx.buffer.substr(0, pos);
	line = headerSplit(headerLines, ' ');
	for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); ++it)
	{
		colon = it->find(":");
		/** to be done */
	}
}

RequestState	Request::state() const { return (_state); }
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
