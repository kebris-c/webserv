/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/16 19:00:50 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

int	main()
{
	Request request = Request();

	request.feed(
		"GET /index.html HTTP/ 1.1\r\n"
		"Host: localhost:8080\r\n"
		"Connection: close\r\n"
		"\r\n"
	);
	request.print();
}
