/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/10/06 16:10:00 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Lexer.hpp"
#include "Router.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "RequestParser.hpp"

int	main(int ac, char* av[])
{
	Config			config;
	Request			request;
	RequestParser	parser;
	Router			router;
	RouteMatch		rMatch;
	std::vector<std::string>	simulatedFeed;
	std::vector<ServerConfig>	server;

	(void)ac;
	config.load(av[1]);
	server = config.servers();
	request.setServer(server);
	simulatedFeed.push_back("GET /index.html");
	simulatedFeed.push_back(" HTTP/1.1\r\n");
	simulatedFeed.push_back("Host: localhost:8080\r\n");
	simulatedFeed.push_back("Transfer-Encoding: chunked\r\n");
	simulatedFeed.push_back("\r\n");
	simulatedFeed.push_back("4\r\n");
	simulatedFeed.push_back("Wiki\r\n");
	simulatedFeed.push_back("5\r\n");
	simulatedFeed.push_back("pedia\r\n");
	simulatedFeed.push_back("E\r\n");
	simulatedFeed.push_back(" in\r\n");
	simulatedFeed.push_back("\r\n");
	simulatedFeed.push_back("chunks.\r\n");
	simulatedFeed.push_back("0\r\n");
	simulatedFeed.push_back("\r\n");

	for (std::vector<std::string>::iterator it = simulatedFeed.begin();
		it != simulatedFeed.end();
		++it)
	{
		std::cout << *it << std::endl;
		if (!request.feed(*it, parser))
		{
			std::cerr << "Error 400: Bad request" << std::endl;
			return (1);
		}
	}
	if (!request.isComplete())
		return (request.state() != REQ_ERROR);
	rMatch = router.match(server[0], request);
	return (0);
}
