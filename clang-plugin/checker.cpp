/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Tartan
 * Copyright © 2014 Collabora Ltd.
 *
 * Tartan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tartan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tartan.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Philip Withnall <philip.withnall@collabora.co.uk>
 */

#include <unordered_set>

#include "checker.h"

namespace tartan {

bool
Checker::is_enabled () const
{
	/* Run away if the plugin is disabled. */
	return (this->_disabled_plugins.get ()->find (this->get_name ()) ==
	        this->_disabled_plugins.get ()->end () &&
	        this->_disabled_plugins.get ()->find ("all") ==
	        this->_disabled_plugins.get ()->end ());
}

} /* namespace tartan */
