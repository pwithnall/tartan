/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * gnome-clang
 * Copyright © 2013 Collabora Ltd.
 *
 * gnome-clang is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gnome-clang is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gnome-clang.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Philip Withnall <philip.withnall@collabora.co.uk>
 */

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>

#include <girepository.h>

using namespace clang;

class GirAttributesConsumer : public ASTConsumer {
private:
	std::string _gi_namespace;
	std::string _gi_version;
	std::string _gi_c_prefix;

	GIRepository *_repo = NULL;
	GITypelib *_typelib = NULL;

public:
	GirAttributesConsumer (std::string& gi_namespace,
	                       std::string& gi_version);
	~GirAttributesConsumer ();

	void prepare (GError **error);

private:
	void _handle_function_decl (FunctionDecl& func);
public:
	virtual bool HandleTopLevelDecl (DeclGroupRef decl_group);
};
