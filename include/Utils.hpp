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

std::string	toString(int n);
std::string	statusReason(int code);

} /* namespace utils */

#endif /* UTILS_HPP */
