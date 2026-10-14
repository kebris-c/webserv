/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kjroydev <kjroydev@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 21:57:18 by kmarrero          #+#    #+#             */
/*   Updated: 2026/10/14 22:46:44 by kjroydev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "Server.hpp"

#include <iostream>
#include <vector>

int main(int ac, char **av)
{
    Config                       config;
    std::vector<ServerConfig>    servers;
    Server                       server;

    if (ac != 2)
    {
        std::cerr << "Usage: ./webserv <config.conf>"
                  << std::endl;
        return (1);
    }

    // Cargar configuracion
    if (config.load(av[1]) != true)
    {
        std::cerr << "Error loading configuration"
                  << std::endl;
        return (1);
    }

    servers = config.servers();

    if (servers.empty())
    {
        std::cerr << "No servers configured"
                  << std::endl;
        return (1);
    }

    // Configurar sockets y puertos
    if (!server.configure(servers))
    {
        std::cerr << "Error configuring server"
                  << std::endl;
        return (1);
    }

    std::cout << "Webserv running..." << std::endl;

    // Iniciar el bucle de eventos
    return (server.run());
}
