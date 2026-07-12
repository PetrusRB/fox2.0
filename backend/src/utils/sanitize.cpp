#include "./sanitize.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

extern "C" {
#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/HTMLtree.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/xmlIO.h>
#include <libxml2/libxml/xmlstring.h>
}

namespace Crown {

static const std::set<std::string> kAllowedTags = {
    "html", "head", "body", "p",     "br",    "b",     "i",
    "u",    "strong", "em", "s",     "del",   "ins",   "sub",
    "sup",  "span", "a",   "img",   "h1",    "h2",    "h3",
    "h4",   "h5",   "h6",  "ul",    "ol",    "li",    "blockquote",
    "pre",  "code", "div", "table", "thead", "tbody", "tr",
    "th",   "td",
};

static const std::set<std::string> kAllowedAttributes = {
    "href", "src", "alt", "title", "width", "height", "class",
};

static bool StartsWithCaseInsensitive(const std::string &str,
                                      const std::string &prefix) {
  if (str.size() < prefix.size())
    return false;
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(str[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i])))
      return false;
  }
  return true;
}

static bool IsValidUrl(const std::string &url) {
  std::string lower = url;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower.rfind("javascript:", 0) == 0)
    return false;
  if (lower.rfind("vbscript:", 0) == 0)
    return false;
  if (lower.rfind("data:", 0) == 0)
    return false;
  if (lower.rfind("file:", 0) == 0)
    return false;

  return true;
}

static void SanitizeNode(xmlNode *node) {
  xmlNode *cur = node;
  while (cur) {
    xmlNode *next = cur->next;

    if (cur->type == XML_ELEMENT_NODE) {
      std::string name = reinterpret_cast<const char *>(cur->name);

      std::string lower_name = name;
      std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                     ::tolower);

      if (kAllowedTags.find(lower_name) == kAllowedTags.end()) {
        xmlNode *child = cur->children;
        while (child) {
          xmlNode *child_next = child->next;
          xmlAddPrevSibling(cur, child);
          child = child_next;
        }
        cur->children = nullptr;
        xmlUnlinkNode(cur);
        xmlFreeNode(cur);
        cur = next;
        continue;
      }

      xmlAttr *attr = cur->properties;
      while (attr) {
        xmlAttr *attr_next = attr->next;

        std::string attr_name = reinterpret_cast<const char *>(attr->name);
        std::string lower_attr = attr_name;
        std::transform(lower_attr.begin(), lower_attr.end(), lower_attr.begin(),
                       ::tolower);

        bool should_remove = false;

        if (StartsWithCaseInsensitive(lower_attr, "on")) {
          should_remove = true;
        } else if (lower_attr == "style") {
          should_remove = true;
        } else if (kAllowedAttributes.find(lower_attr) ==
                   kAllowedAttributes.end()) {
          should_remove = true;
        }

        if (should_remove) {
          xmlRemoveProp(attr);
        } else if (lower_attr == "href" || lower_attr == "src") {
          xmlChar *prop = xmlGetProp(
              cur, reinterpret_cast<const xmlChar *>(attr_name.c_str()));
          if (prop) {
            std::string url = reinterpret_cast<const char *>(prop);
            xmlFree(prop);
            if (!IsValidUrl(url)) {
              xmlSetProp(cur,
                         reinterpret_cast<const xmlChar *>(attr_name.c_str()),
                         reinterpret_cast<const xmlChar *>(""));
            }
          }
        }

        attr = attr_next;
      }

      SanitizeNode(cur->children);
    } else if (cur->type == XML_TEXT_NODE) {
    }

    cur = next;
  }
}

std::string SanitizeString(const std::string &input) {
  if (input.empty())
    return "";

  htmlDocPtr doc = htmlReadMemory(
      input.c_str(), static_cast<int>(input.size()), nullptr, nullptr,
      HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

  if (!doc)
    return "";

  xmlNode *root = xmlDocGetRootElement(doc);
  if (!root) {
    xmlFreeDoc(doc);
    return "";
  }

  SanitizeNode(root);

  xmlNode *body = nullptr;
  for (xmlNode *child = root->children; child; child = child->next) {
    if (child->type == XML_ELEMENT_NODE) {
      std::string tag = reinterpret_cast<const char *>(child->name);
      std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
      if (tag == "body") {
        body = child;
        break;
      }
    }
  }
  if (!body)
    body = root;

  std::string result;
  xmlBufferPtr buf = xmlBufferCreate();
  for (xmlNode *child = body->children; child; child = child->next) {
    xmlNodeDump(buf, child->doc, child, 0, 0);
    result += reinterpret_cast<const char *>(xmlBufferContent(buf));
    xmlBufferEmpty(buf);
  }
  xmlBufferFree(buf);
  xmlFreeDoc(doc);

  size_t start = result.find_first_not_of(" \t\n\r");
  if (start == std::string::npos)
    return "";
  size_t end = result.find_last_not_of(" \t\n\r");
  return result.substr(start, end - start + 1);
}

std::string SanitizeUrl(const std::string &input) {
  std::string trimmed = input;
  auto start = trimmed.find_first_not_of(" \t\n\r");
  if (start == std::string::npos)
    return "";
  auto end = trimmed.find_last_not_of(" \t\n\r");
  trimmed = trimmed.substr(start, end - start + 1);

  std::string lower = trimmed;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  static const std::vector<std::string> kBlockedSchemes = {
      "javascript:", "vbscript:", "data:", "file:", "blob:",
  };

  for (const auto &scheme : kBlockedSchemes) {
    if (lower.rfind(scheme, 0) == 0)
      return "";
  }

  return trimmed;
}

std::string DecodeUnicodeEscapes(const std::string &input) {
  std::string result;
  result.reserve(input.size());
  size_t i = 0;

  while (i < input.size()) {
    if ((input[i] == '\\' || input[i] == '/') && i + 5 < input.size() &&
        input[i + 1] == 'u' &&
        std::isxdigit(static_cast<unsigned char>(input[i + 2])) &&
        std::isxdigit(static_cast<unsigned char>(input[i + 3])) &&
        std::isxdigit(static_cast<unsigned char>(input[i + 4])) &&
        std::isxdigit(static_cast<unsigned char>(input[i + 5]))) {
      std::string hex = input.substr(i + 2, 4);
      unsigned int code = std::stoul(hex, nullptr, 16);
      result += static_cast<char>(code);
      i += 6;
      continue;
    }
    if (input[i] == 'u' && i + 4 < input.size() &&
        std::isxdigit(static_cast<unsigned char>(input[i + 1])) &&
        std::isxdigit(static_cast<unsigned char>(input[i + 2])) &&
        std::isxdigit(static_cast<unsigned char>(input[i + 3])) &&
        std::isxdigit(static_cast<unsigned char>(input[i + 4]))) {
      std::string hex = input.substr(i + 1, 4);
      unsigned int code = std::stoul(hex, nullptr, 16);
      result += static_cast<char>(code);
      i += 5;
      continue;
    }
    result += input[i];
    i++;
  }
  return result;
}

} // namespace Crown
