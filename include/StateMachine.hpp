/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StateMachine.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 19:46:24 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/07 20:32:32 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STATEMACHINE_HPP
# define STATEMACHINE_HPP

# include "Webserv.hpp"
# include <memory>

enum States
{
	START,
	WORD,
	READING,
	RBRACET,
	LBRACET,
	ERROR,
	END
};

enum Event
{
	BALANCE,
	KEYWORD,
	LINE,
	EOF_REACHED,
	ERROR
};

struct	Context
{
	std::string	line;
	int			lineNumber;
};

class	State
{
	public:
		virtual ~State() = default;
		virtual	State* handle(const Event& event, Context& context) = 0;
};

class	StateMachine
{
	private:
		Context					context;
		std::unique_ptr<State>	state;
	public:
		
};

#endif