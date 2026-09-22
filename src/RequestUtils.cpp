/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:42:41 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/22 19:53:02 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestUtils.hpp"

bool	checkVersion(std::string& word)
{
	std::string::size_type	pos;
	std::string				version;

	pos = word.find("/");
	version = word.substr(pos + 1);
	for (size_t i = 0; i < version.size(); i++)
	{
		if (!isdigit(version[i]))
			return (false);
	}
	return (true);
}

bool	checkMethod(std::string& word)
{
	if (word != "GET" && word != "POST" && word != "DELETE")
		return (false);
	return (true);
}

bool	checkTarget(std::string& word)
{
	if (word[0] == '/')
		return (true);
	return (false);
}
