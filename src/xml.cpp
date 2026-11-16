// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "xml.hpp"

#include <ostream>

#include "debug.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
// In cases like getting an attribute or a child element, if the given name
// string is empty, we should not call the tinyxml2 function with the c string
// of the empty name string, but rather with a nullptr. Use this function to
// convert std::string's to c strings, rather than std::string::c_str().
static const char* to_c_str(const std::string& str)
{
    return str.empty() ? nullptr : str.c_str();
}

// -----------------------------------------------------------------------------
// xml
// -----------------------------------------------------------------------------
namespace xml
{
void load_file(const std::string& path, Doc& to_doc)
{
    TRACE << "Loading xml file at: " << path << "\n";

    const tinyxml2::XMLError load_result = to_doc.LoadFile(path.c_str());

    if (load_result != tinyxml2::XML_SUCCESS) {
        TRACE_ERROR_RELEASE
            << "Failed to find or read xml file at: "
            << path
            << "\n"
            << "tinyxml2 reported error: "
            << to_doc.ErrorName()
            << "\n";

        PANIC;
    }
}

Element* child(Doc& doc)
{
    return doc.FirstChildElement();
}

Element* child(Element* const e, const std::string& name)
{
    return e->FirstChildElement(to_c_str(name));
}

bool has_child(Element* e, const std::string& name)
{
    return e->FirstChildElement(to_c_str(name)) != nullptr;
}

Element* next(Element* e, const std::string& name)
{
    return e->NextSiblingElement(to_c_str(name));
}

std::string get_str(const Element* const e)
{
    std::string str;

    const char* c_str = e->GetText();

    if (c_str) {
        str = c_str;
    }

    return str;
}

bool get_bool(const Element* const e)
{
    bool value = false;

    const tinyxml2::XMLError result = e->QueryBoolText(&value);

    if (result != tinyxml2::XML_SUCCESS) {
        TRACE_ERROR_RELEASE
            << "While parsing boolean value from "
            << "xml element \""
            << e->Value()
            << "\", tinyxml2 reported error code: "
            << result << "\n";

        PANIC;
    }

    return value;
}

int get_int(const Element* const e)
{
    int value = false;

    const tinyxml2::XMLError result = e->QueryIntText(&value);

    if (result != tinyxml2::XML_SUCCESS) {
        TRACE_ERROR_RELEASE
            << "While parsing integer value from "
            << "xml element \""
            << e->Value()
            << "\", tinyxml2 reported error code: "
            << result << "\n";

        PANIC;
    }

    return value;
}

std::string get_attribute_str(const Element* const e, const std::string& name)
{
    return e->Attribute(to_c_str(name));
}

int get_attribute_int(const Element* const e, const std::string& name)
{
    int value = 0;

    const int conv_result = e->QueryAttribute(to_c_str(name), &value);

    if (conv_result != tinyxml2::XML_SUCCESS) {
        TRACE_ERROR_RELEASE
            << "While parsing integer value from xml element \""
            << e->Value()
            << "\", attribute \""
            << name
            << "\", tinyxml2 reported error code: "
            << conv_result << "\n";

        PANIC;
    }

    return value;
}

bool get_attribute_bool(const Element* e, const std::string& name)
{
    bool value = false;

    auto conv_result = e->QueryAttribute(name.c_str(), &value);

    if (conv_result != tinyxml2::XML_SUCCESS) {
        TRACE_ERROR_RELEASE
            << "While parsing boolean value from xml element \""
            << e->Value()
            << "\", attribute \""
            << name
            << "\", tinyxml2 reported error code: "
            << conv_result << "\n";

        PANIC;
    }

    return value;
}

bool has_attribute_str(const Element* e, const std::string& name)
{
    const char* str = e->Attribute(name.c_str());

    return str != nullptr;
}

bool has_attribute_int(const Element* e, const std::string& name)
{
    int placeholder = 0;

    auto result = e->QueryAttribute(name.c_str(), &placeholder);

    return result == tinyxml2::XML_SUCCESS;
}

bool has_attribute_bool(const Element* e, const std::string& name)
{
    bool placeholder = false;

    auto result = e->QueryAttribute(name.c_str(), &placeholder);

    return result == tinyxml2::XML_SUCCESS;
}

}  // namespace xml
