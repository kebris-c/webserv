/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 16:05:56 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 21:55:56 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_PARSER_HPP
# define REQUEST_PARSER_HPP

# include "Webserv.hpp"
# include "RStateMachine.hpp"
# include "Request.hpp"

typedef	RequestState	(*RequestHelpFunction)(const std::string&, RequestContext&, Request&);

class	RequestParser
{
	private:
		std::map<std::string, RequestHelpFunction>		headerHelpers;
		std::vector<std::string>			requestSplit(const std::string& str, char delimeter);
		std::vector<std::string>			headerSplit(const std::string& str);
		RequestState						headersChecker(RequestContext& ctx, Request& request);
	public:
		RequestParser();
		~RequestParser();
		RequestState						checkFeed(RequestContext& ctx, Request& request);
		RequestState						parseRequestLine(RequestContext& ctx, Request& request);
		RequestState						parseRequestHeader(RequestContext& ctx, Request& request);
		RequestState						parseRequestBody(RequestContext& ctx, Request& request);
		std::vector<std::string>			bodyChunkConstruct(RequestContext& ctx);
};

#endif
