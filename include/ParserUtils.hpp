/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParserUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:38:24 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/16 14:29:16 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_UTILS_HPP
# define PARSER_UTILS_HPP

# include "Webserv.hpp"
# include "Parser.hpp"

int							checkFileExistence(const std::string& path, Context& ctx);
int							checkDirectoryExistence(const std::string& path, Context& ctx);
int							checkEndFile(Context& ctx, ParserState state, std::string message);
int							checkNextValue(Context& ctx, ParserState state, std::string message);
void						setError(std::string message, Context& ctx, ParserState state);
bool						isValidIP(std::string ip, Context& ctx);
bool						isValidPort(std::string port, Context& ctx);
bool						isValidRoot(std::string prefix, Context& ctx);
bool						isValidMethod(std::string method, Context& ctx);
bool						isValidErrorCode(std::string error, Context& ctx);
bool						isValidHTML(std::string htmlFileName, Context& ctx);
std::string::size_type		isValidClientSize(std::string word, Context& ctx);
ParserState					checkNextElement(Context& ctx, Parser& parser);

#endif