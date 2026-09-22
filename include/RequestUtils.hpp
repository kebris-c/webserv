/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 18:43:24 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/22 19:20:52 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_UTILS_HPP
# define REQUEST_UTILS_HPP

# include "Webserv.hpp"
# include "Request.hpp"

bool	checkVersion(std::string& word);
bool	checkMethod(std::string& word);
bool	checkTarget(std::string& word);

#endif