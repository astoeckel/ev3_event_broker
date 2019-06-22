/**
 *  EV3 Event Broker -- Talk to Lego Robots using UDP
 *  Copyright (C) 2019  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file argparse.hpp
 *
 * A truely minimal argument parser. Parses arguments of the form "--arg FOO"
 * and "--arg=FOO". Displays a help on "--help" and "-h".
 *
 * @author Andreas Stöckel
 */

#pragma once

#include <functional>
#include <memory>

namespace ev3_event_broker {
/**
 * The Argparse class implements a simple command line argument parser.
 * Arguments can be registered using the register_arg() member function. Parsing
 * is triggered by calling the parse() member function.
 */
class Argparse {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	/**
	 * Callback called for each registered argument. This is called exactly once
	 * per argument if either a default was specified or the user gave the
	 * argument. May return false to cancel the parsing process.
	 */
	using Callback = std::function<bool(const char *value)>;

	/**
	 * Creates a new Argparse instance.
	 *
	 * @param prog_name is the name of the program executable.
	 * @param prog_descr is the description of the program to be printed in the
	 * usage information.
	 */
	Argparse(const char *prog_name, const char *prog_descr);

	/**
	 * Destroys the Argparse instance. In particlar, destroys the internal
	 * m_impl instance.
	 */
	~Argparse();

	/**
	 * Registers a new argument.
	 *
	 * @param name of the argument. Should not include "-" or "--".
	 * @param descr is the description of the argument to be printed in the
	 * usage information.
	 * @param default_ is the default value that should be assigned to the
	 * argument. May be nullptr. In this case the callback is not called if the
	 * argument is not explicitly specified by the user.
	 * @param cback is the callback function that should be called when either
	 * the argument has been specified by the user or the default value is being
	 * used.
	 * @return a reference at this Argparse instance for convenient chaining.
	 */
	Argparse &add_arg(const char *name, const char *descr, const char *default_,
	                  const Callback &cback);

	/**
	 * Adds a switch. The callback function is only called when the switch is
	 * specified by the user.
	 */
	Argparse &add_switch(const char *name, const char *descr,
	                     const Callback &cback);

	/**
	 * Parses the given command line arguments.
	 */
	void parse(int argc, const char *argv[]) const;
};

}  // namespace ev3_event_broker
