/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 18:38:24 by kjroydev          #+#    #+#             */
/*   Updated: 2026/09/13 21:04:48 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include "Webserv.hpp"
# include "Parser.hpp"

int							checkEndFile(Parser& parser, Context& ctx, ParserState state, std::string message);
void						setError(std::string message, Context& ctx, Parser& parser, ParserState state);
bool						isValidIP(std::string ip, Context& ctx, Parser& parser);
bool						isValidPort(std::string port, Context& ctx, Parser& parser);
std::string::size_type		isValidClientSize(std::string word, Context& ctx, Parser& parser);
ParserState					checkNextElement(Context& ctx, Parser& parser);

#endif