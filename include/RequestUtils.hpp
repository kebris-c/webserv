/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:43:24 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 21:15:26 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_UTILS_HPP
# define REQUEST_UTILS_HPP

# include "Webserv.hpp"
# include "Request.hpp"

bool			checkVersion(const std::string& word);
bool			checkMethod(const std::string& word);
bool			checkTarget(const std::string& word);
bool			checkNameHeader(std::string& name, RequestContext& ctx);
bool			checkValueHeader(std::string& value, RequestContext& ctx);
void			prepareTarget(const std::string& target, Request& request);
RequestState	contentLengthHeader(const std::string& header, RequestContext& ctx, Request& request);
RequestState	transferEncodingHeader(const std::string& header, RequestContext& ctx, Request& request);
RequestState	connectionHeader(const std::string& header, RequestContext& ctx, Request& request);

#endif