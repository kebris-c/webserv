/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfParserUtils.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:38:24 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/29 23:45:07 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONF_PARSER_UTILS_HPP
# define CONF_PARSER_UTILS_HPP

# include "Webserv.hpp"
# include "ConfParser.hpp"

int							checkFileExistence(const std::string& path, ParserContext& ctx);
int							checkDirectoryExistence(const std::string& path, ParserContext& ctx);
int							checkEndFile(ParserContext& ctx, ParserState state, std::string message);
int							checkNextValue(ParserContext& ctx, ParserState state, std::string message);
void						setError(std::string message, ParserContext& ctx, ParserState state);
bool						isValidIP(std::string ip, ParserContext& ctx);
bool						isValidPort(std::string port, ParserContext& ctx);
bool						isValidRoot(std::string prefix, ParserContext& ctx);
bool						isValidMethod(std::string method, ParserContext& ctx);
bool						isValidErrorCode(std::string error, ParserContext& ctx);
bool						isValidHTML(std::string htmlFileName, ParserContext& ctx);
size_t						convertToBytes(size_t number, char size);
std::string::size_type		isValidClientSize(std::string word, ParserContext& ctx);
ParserState					checkNextElement(ParserContext& ctx, Parser& parser);

#endif