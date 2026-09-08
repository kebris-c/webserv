/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 22:37:17 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/08 18:57:23 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"

void	Parser::balance(Context& ctx)
{
	std::vector<Token>	tokens = ctx.tokens;
	int	counter = 0;

	for (unsigned int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i].value == "{" || tokens[i].value == "}")
			counter += 1;
	}
	if (counter % 2)
	{
		std::cout << "Current .conf bracets are correctly balanced";
		ctx.balance = true;
	}
	else
	{
		std::cerr << "Error!" << std::endl;
		ctx.balance = false;
	}
}
