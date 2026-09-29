/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:43:24 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/29 18:39:58 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_UTILS_HPP
# define REQUEST_UTILS_HPP

# include "Request.hpp"

bool					checkVersion(const std::string& word);
bool					checkMethod(const std::string& word);
bool					checkTarget(const std::string& word);
bool					checkNameHeader(std::string& name, RequestContext& ctx);
bool					checkValueHeader(std::string& value, RequestContext& ctx);
void					prepareTarget(const std::string& target, Request& request);
RequestState			contentLengthHeader(const std::string& header, RequestContext& ctx, Request& request);
RequestState			transferEncodingHeader(const std::string& header, RequestContext& ctx, Request& request);
RequestState			connectionHeader(const std::string& header, RequestContext& ctx, Request& request);
std::string::size_type	obtainStatusFromContext(Request& request);
std::string::size_type	obtainBodyInfo(Request& request);
int						obtainHexValue(std::string& value);
int						bufferConstruct(size_t& hexValue, std::string& buffer, std::vector<std::string>& phrases,
											size_t& i);

#endif