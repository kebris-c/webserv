/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   States.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmarrero <kmarrero@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 19:56:35 by kmarrero          #+#    #+#             */
/*   Updated: 2026/09/07 20:24:33 by kmarrero         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STATES_HPP
# define STATES_HPP

# include "Webserv.hpp"
# include "StateMachine.hpp"

class	StateStart: public State
{
	virtual State*	handle(const Event& event, Context& context);
};

class	StateWord: public State
{
	virtual State*	handle(const Event& event, Context& context);
};

class	StateReading: public State
{
	virtual State*	handle(const Event& event, Context& context);
};

class	StateRBracet: public State
{
	virtual State*	handle(const Event& event, Context& context);
};

class	StateLBracet: public State
{
	virtual State*	handle(const Event& event, Context& context);
};

class	StateError: public State
{
	virtual State*	handle(const Event& event, Context& context);
};

class	StateEnd: public State
{
	virtual State*	handle(const Event& event, Context& context);
};

#endif