/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:38:24 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/14 19:03:17 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include "Webserv.hpp"
# include "Parser.hpp"

int							checkEndFile(Parser& parser, Context& ctx, ParserState state, std::string message);
int							checkNextValue(Parser& parser, Context& ctx, ParserState state, std::string message);
void						setError(std::string message, Context& ctx, Parser& parser, ParserState state);
bool						isValidIP(std::string ip, Context& ctx, Parser& parser);
bool						isValidPort(std::string port, Context& ctx, Parser& parser);
bool						isValidRoot(std::string prefix, Context& ctx, Parser& parser);
bool						isValidMethod(std::string method, Context& ctx, Parser& parser);
bool						isValidErrorCode(std::string error, Context& ctx, Parser& parser);
bool						isValidHTML(std::string htmlFileName, Context& ctx, Parser& parser);
std::string::size_type		isValidClientSize(std::string word, Context& ctx, Parser& parser);
ParserState					checkNextElement(Context& ctx, Parser& parser);

#endif