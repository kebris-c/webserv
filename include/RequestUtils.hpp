/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:43:24 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/23 15:59:21 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_UTILS_HPP
# define REQUEST_UTILS_HPP

# include "Webserv.hpp"
# include "Request.hpp"

bool			checkVersion(std::string& word);
bool			checkMethod(std::string& word);
bool			checkTarget(std::string& word);
bool			checkNameHeader(std::string& name, Context& ctx);
bool			checkValueHeader(std::string& value, Context& ctx);
RequestState	contentLenghtHeader(std::string& header, Context& ctx, Request& request);
RequestState	transferEncodingHeader(std::string& header, Context& ctx, Request& request);
RequestState	connectionHeader(std::string& header, Context& ctx, Request& request);

#endif