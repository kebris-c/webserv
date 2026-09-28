/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 16:05:56 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/28 18:46:34 by kjroydev         ###   ########.fr       */
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
		std::vector<std::string>			headerSplit(Request& request);
		std::vector<std::string>			bodySplit(Request& request);
		RequestState						headersChecker(RequestContext& ctx, Request& request);
		std::string							parseChunkedBody(std::vector<std::string>& phrases, Request& request);
	public:
		RequestParser();
		~RequestParser();
		RequestState						checkFeed(RequestContext& ctx, Request& request);
		RequestState						parseRequestLine(RequestContext& ctx, Request& request);
		RequestState						parseRequestHeader(RequestContext& ctx, Request& request);
		RequestState						parseRequestBody(RequestContext& ctx, Request& request);
};

#endif
