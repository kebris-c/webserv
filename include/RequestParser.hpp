/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 16:05:56 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/25 17:49:46 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_PARSER_HPP
# define REQUEST_PARSER_HPP

# include "Webserv.hpp"
# include "RStateMachine.hpp"
# include "Request.hpp"

typedef	RequestState	(*Function)(const std::string&, RequestContext&, Request&);

class	RequestParser
{
	private:
		std::map<std::string, Function>		headerHelpers;
		std::vector<std::string>			split(const std::string& str, char delimeter);
		std::vector<std::string>			headerSplit(const std::string& str);
		void								prepareTarget(const std::string& target, Request& request);
		RequestState						headersChecker(RequestContext& ctx, Request& request);
	public:
		RequestParser();
		~RequestParser();
		RequestState						parseRequestLine(RequestContext& ctx, Request& request);
		RequestState						parseRequestHeader(RequestContext& ctx, Request& request);
		RequestState						parseRequestBody(RequestContext& ctx, Request& request);
		std::vector<std::string>			bodyChunkConstruct(RequestContext& ctx);
		void								parseTarget(std::string& target);
};

#endif
