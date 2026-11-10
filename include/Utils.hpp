/* **************************************************************************
 * Utils.hpp
 * OWNER: both (small shared helpers; do not grow into a junk drawer)
 * GOAL: Path join, string trim/split, status text, file existence helpers.
 * LEARN: C++98 string ops; safe path concatenation without path traversal.
 * ============================================================================ */

#ifndef UTILS_HPP
#define UTILS_HPP

#include "Webserv.hpp"

namespace utils {

std::string	trim(const std::string &s);
std::vector<std::string>	split(const std::string &s, char delim);
std::string	toString(int n);
std::string	statusReason(int code);
bool		fileExists(const std::string &path);
bool		isDirectory(const std::string &path);
std::string	joinPath(const std::string &a, const std::string &b);

/* TODO(both): reject ".." path segments when mapping URL -> filesystem. */

} /* namespace utils */

#endif /* UTILS_HPP */
