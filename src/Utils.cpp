/* **************************************************************************
 * Utils.cpp
 * OWNER: both
 * ************************************************************************** */

#include "Utils.hpp"

namespace utils {

std::string	trim(const std::string &s)
{
	/* TODO(both): strip whitespace from both ends */
	(void)s;
	return (std::string());
}

std::vector<std::string>	split(const std::string &s, char delim)
{
	/* TODO(both) */
	(void)s;
	(void)delim;
	return (std::vector<std::string>());
}

std::string	toString(int n)
{
	std::ostringstream	oss;
	oss << n;
	return (oss.str());
}

std::string	statusReason(int code)
{
	/* TODO(kmarrero): map common codes — 200/201/204/301/302/400/403/404/405/413/500/501 */
	switch (code)
	{
	case 200: return "OK";
	case 404: return "Not Found";
	case 500: return "Internal Server Error";
	default:  return "Unknown";
	}
}

bool	fileExists(const std::string &path)
{
	/* TODO(both): stat() */
	(void)path;
	return (false);
}

bool	isDirectory(const std::string &path)
{
	/* TODO(both): S_ISDIR */
	(void)path;
	return (false);
}

std::string	joinPath(const std::string &a, const std::string &b)
{
	/* TODO(both): avoid duplicate slashes; do not resolve ".." yet — reject instead */
	if (a.empty())
		return (b);
	if (b.empty())
		return (a);
	if (a[a.size() - 1] == '/')
		return (a + b);
	return (a + "/" + b);
}

} /* namespace utils */
