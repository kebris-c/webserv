/* **************************************************************************
 * Request.cpp
 * OWNER: kmarrero
 * ************************************************************************** */

#include "Request.hpp"
#include "RequestUtils.hpp"

Request::Request()
	:_state(REQ_FEED), _contentLength(0), _chunked(false), _errorCode(0)
{
	headerHelpers["Content-Lenght"] = &contentLenghtHeader;
	headerHelpers["Transfer-Encoding"] = &transferEncodingHeader;
	headerHelpers["Connection"] = &connectionHeader;
}

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

void	Request::setError(std::string message, Context& ctx, RequestState state, int errorCode)
{
	_state = state;
	_errorCode = errorCode;
	if (ctx.error.empty())
		ctx.error = message;
	else
		ctx.error = message + ctx.error;
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
		return (setError("Invalid header", ctx, REQ_ERROR, 400), REQ_ERROR);
	_method = line[0];
	if (!checkMethod(_method))
		return (setError("No valid method", ctx, REQ_ERROR, 400), REQ_ERROR);
	_target = line[1];
	if (!checkTarget(_target))
		return (setError("Invalid target format", ctx, REQ_ERROR, 400), REQ_ERROR);
	parseTarget(_target);
	_version = line[2];
	if (!checkVersion(_version))
		return (setError("version error", ctx, REQ_ERROR, 400), REQ_ERROR);
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

void	Request::parseHeaders(Context& ctx)
{
	RequestState	answer;
	std::map<std::string, Function>::iterator	loc;
	std::map<std::string, std::string>::iterator	host;

	host = _headers.find("Host");
	if (host == _headers.end())
	{
		setError("HEADER: Host not found", ctx, REQ_ERROR, 400);
		return ;
	}
	for (std::map<std::string, std::string>::iterator it = _headers.begin();
			it != _headers.end(); ++it)
	{
		loc = headerHelpers.find(_headers[it->first]);
		if (loc == headerHelpers.end())
			continue ;
		answer = loc->second(it->second, ctx);
	}
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
	line = headerSplit(headerLines);
	for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); ++it)
	{
		colon = it->find(":");
		if (colon == 0 || (*it)[colon - 1] == ' ' || colon == std::string::npos)
			return (setError("HEADER: colon (:) not found", ctx, REQ_ERROR, 400), REQ_ERROR);
		name = it->substr(0, colon);
		if (!checkNameHeader(name, ctx))
			return (setError("HEADER NAME: ", ctx, REQ_ERROR, 400), REQ_ERROR);
		value = it->substr(colon + 1);
		if (!checkValueHeader(value, ctx))
			return (setError("HEADER VALUE: ", ctx, REQ_ERROR, 400), REQ_ERROR);
		_headers[name] = value;
	}
	parseHeaders(ctx);
	return (REQ_BODY);
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
