/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParserUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:38:24 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/25 17:47:32 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_UTILS_HPP
# define PARSER_UTILS_HPP

# include "Webserv.hpp"
# include "Parser.hpp"

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