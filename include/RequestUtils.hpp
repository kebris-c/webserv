/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:43:24 by kmarrero          #+#    #+#             */
/*   Updated: 2026/10/17 01:42:09 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_UTILS_HPP
# define REQUEST_UTILS_HPP

# include "Request.hpp"

bool					checkTarget(const std::string& word);
bool					checkMethod(const std::string& word);
bool					checkVersion(const std::string& word);
bool					normalizeHeader(const std::string& header);
bool					checkNameHeader(std::string& name, RequestContext& ctx);
bool					checkValueHeader(std::string& value, RequestContext& ctx);
void					prepareTarget(const std::string& target, Request& request);
RequestState			connectionHeader(const std::string& header, RequestContext& ctx, Request& request);
RequestState			contentLengthHeader(const std::string& header, RequestContext& ctx, Request& request);
RequestState			transferEncodingHeader(const std::string& header, RequestContext& ctx, Request& request);
std::string::size_type	obtainBodyInfo(Request& request);
std::string::size_type	obtainStatusFromContext(Request& request);
int						obtainHexValue(std::string& value);
int						bufferConstruct(size_t& hexValue,
											std::string& buffer,
											std::vector<std::string>& phrases,
											size_t& i);
										

#endif