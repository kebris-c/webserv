/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiUtils.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 16:04:08 by kjroydev          #+#    #+#             */
/*   Updated: 2026/10/31 16:44:28 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_UTILS_HPP
# define CGI_UTILS_HPP

# include "Webserv.hpp"
# include "CgiProcess.hpp"

typedef std::multimap<std::string, std::string> CgiHeaders;

bool	readCgiHeaders(const std::string &output, CgiHeaders &headers,
					std::size_t &bodyStart);
bool	cgiConnectionFields(const CgiHeaders &headers,
						std::map<std::string, bool> &excluded);
bool	validCgiName(const std::string &name);
bool	parseCgiHeader(const std::string &line, CgiHeaders &headers);
bool	parseCgiStatus(const std::string &value, int &status);
bool	parseCgiLength(const std::string &value, std::size_t &length);
bool	cgiHopByHop(const std::string &name);
std::string	lowerCgiName(std::string name);
std::string	trimCgiValue(const std::string &value);

#endif