// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef XML_HPP
#define XML_HPP

#include "tinyxml2.h"
#include <string>

namespace xml
{
using Doc = tinyxml2::XMLDocument;
using Element = tinyxml2::XMLElement;

void load_file(const std::string& path, Doc& to_doc);

Element* child(Doc& doc);
Element* child(Element* e, const std::string& name = "");

bool has_child(Element* e, const std::string& name);

Element* next(Element* e, const std::string& name = "");

std::string get_str(const Element* e);
bool get_bool(const Element* e);
int get_int(const Element* e);

std::string get_attribute_str(const Element* e, const std::string& name);
int get_attribute_int(const Element* e, const std::string& name);
bool get_attribute_bool(const Element* e, const std::string& name);

bool has_attribute_str(const Element* e, const std::string& name);
bool has_attribute_int(const Element* e, const std::string& name);
bool has_attribute_bool(const Element* e, const std::string& name);

}  // namespace xml

#endif  // XML_HPP
