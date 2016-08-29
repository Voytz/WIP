#include "json_parse.h"

#include <cctype>
#include <istream>
#include <string>
#include <unordered_map>
#include "json_node_array.h"
#include "json_node_bool.h"
#include "json_node_null.h"
#include "json_node_number.h"
#include "json_node_object.h"
#include "json_node_string.h"
#include "json_parse.h"
#include "map.h"

json_parse_context::json_parse_context(std::istream &stream)
    : m_stream(stream) {}

bool json_parse_context::ok() const { return m_stream.good(); }

char json_parse_context::c() const { return m_stream.peek(); }

void json_parse_context::next() {
  char tmp;
  m_stream.read(&tmp, 1);
}

void json_parse_context::skipWS() {
  while (ok() && std::isspace(c())) {
    next();
  }
}

std::string encode_json_string(const std::string &value) {
  std::string result;
  result += "\"";
  const std::unordered_map<char, char> tr = {
      {'\\', '\\'}, {'\r', 'r'}, {'\n', 'n'},
      {'\v', 'v'},  {'\t', 't'}, {'"', '"'},
  };
  for (char c : value) {
    if (tr.find(c) != tr.end()) {
      std::string add("\\");
      add += tr.at(c);
      result += add;
    } else {
      result += c;
    }
  }
  result += "\"";
  return result;
}

void dump_object(std::ostream &stream, const json_node_object &node) {
  stream << "{";
  const json_node_object::map_t &values = node.values();
  bool first = true;
  values.fold([&stream, &first](const std::string &key, const s_node &value) {
    if (first) {
      first = false;
    } else {
      stream << ",";
    }
    stream << encode_json_string(key);
    stream << ":";
    dump_any(stream, *value);
  });
  stream << "}";
}
void dump_array(std::ostream &stream, const json_node_array &node) {
  stream << "[";
  const json_node_array::map_t &values = node.values();
  bool first = true;
  values.fold([&stream, &first](size_t, const s_node &value) {
    if (first) {
      first = false;
    } else {
      stream << ",";
    }
    dump_any(stream, *value);
  });
  stream << "]";
}

void dump_number(std::ostream &stream, const json_node_number &node) {
  if (node.is_double()) {
    stream << std::to_string(node.get_double());
  } else {
    stream << std::to_string(node.get_int());
  }
}

void dump_bool(std::ostream &stream, const json_node_bool &node) {
  if (node.value()) {
    stream << "true";
  } else {
    stream << "false";
  }
}

void dump_any(std::ostream &stream, const json_node &node) {
  if (node.type() == json_type_object) {
    dump_object(stream, static_cast<const json_node_object &>(node));
  } else if (node.type() == json_type_string) {
    stream << encode_json_string(
        static_cast<const json_node_string &>(node).value());
  } else if (node.type() == json_type_number) {
    dump_number(stream, static_cast<const json_node_number &>(node));
  } else if (node.type() == json_type_array) {
    dump_array(stream, static_cast<const json_node_array &>(node));
  } else if (node.type() == json_type_bool) {
    dump_bool(stream, static_cast<const json_node_bool &>(node));
  } else if (node.type() == json_type_null) {
    dump_null(stream);
  }
}

void dump_null(std::ostream &stream) { stream << "null"; }

s_node parse_object(json_parse_context &ctx) {
  ctx.skipWS();
  if (!ctx.ok()) {
    return nullptr;
  }
  if (ctx.c() != '{') {
    return nullptr;
  }
  ctx.next();
  if (!ctx.ok()) {
    return nullptr;
  }
  ctx.skipWS();
  json_node_object::map_t values;
  while (true) {
    if (!ctx.ok()) {
      return nullptr;
    }
    if (ctx.c() == '}') {
      ctx.next();
      return s_node(new json_node_object(std::move(values)));
    }
    s_node k = parse_string(ctx);
    if (!k) {
      return nullptr;
    }
    if (!ctx.ok()) {
      return nullptr;
    }
    if (ctx.c() != ':') {
      return nullptr;
    }
    ctx.next();
    s_node v = parse_any(ctx);
    if (!v) {
      return nullptr;
    }
    values.insert(std::static_pointer_cast<const json_node_string>(k)->value(),
                  std::move(v));
    if (!ctx.ok()) {
      return nullptr;
    }
    ctx.skipWS();
    if (!ctx.ok()) {
      return nullptr;
    }
    if (ctx.c() == ',') {
      ctx.next();
    }
    ctx.skipWS();
  }
  return nullptr;
}

s_node parse_array(json_parse_context &ctx) {
  ctx.skipWS();
  if (!ctx.ok()) {
    return nullptr;
  }
  if (ctx.c() != '[') {
    return nullptr;
  }
  ctx.next();
  if (!ctx.ok()) {
    return nullptr;
  }
  ctx.skipWS();
  json_node_array::map_t values;
  while (true) {
    if (!ctx.ok()) {
      return nullptr;
    }
    if (ctx.c() == ']') {
      ctx.next();
      return s_node(new json_node_array(std::move(values)));
    }
    if (!ctx.ok()) {
      return nullptr;
    }
    s_node v = parse_any(ctx);
    if (!v) {
      return nullptr;
    }
    values.insert(values.size(), std::move(v));
    if (!ctx.ok()) {
      return nullptr;
    }
    ctx.skipWS();
    if (!ctx.ok()) {
      return nullptr;
    }
    if (ctx.c() == ',') {
      ctx.next();
    }
    ctx.skipWS();
  }
  return nullptr;
}

s_node parse_string(json_parse_context &ctx) {
  ctx.skipWS();
  if (!ctx.ok()) {
    return nullptr;
  }
  if (ctx.c() != '"') {
    return nullptr;
  }
  ctx.next();
  std::string res;
  const std::unordered_map<char, char> tr = {{'\\', '\\'}, {'r', '\r'},
                                             {'n', '\n'},  {'t', '\t'},
                                             {'v', '\v'},  {'"', '"'}};
  bool escaped = false;
  while (true) {
    if (!ctx.ok()) {
      return nullptr;
    }
    if (escaped) {
      escaped = false;
      if (tr.find(ctx.c()) != tr.end()) {
        res += tr.at(ctx.c());
      } else {
        res += ctx.c();
      }
    } else {
      if (ctx.c() == '\\') {
        escaped = true;
      } else if (ctx.c() == '"') {
        ctx.next();
        return s_node(new json_node_string(std::move(res)));
      } else {
        res += ctx.c();
      }
    }
    ctx.next();
  }
  return nullptr;
}

s_node parse_number(json_parse_context &ctx) {
  ctx.skipWS();
  if (!ctx.ok()) {
    return nullptr;
  }
  std::string rep;
  while (ctx.ok()) {
    if ((ctx.c() >= '0' && ctx.c() <= '9') || ctx.c() == '.') {
      rep += ctx.c();
      ctx.next();
    } else {
      break;
    }
  }
  if (rep.empty()) {
    return nullptr;
  }
  if (rep[0] == '0' && rep.length() > 1 && rep[1] == '0') {
    return nullptr;
  }
  size_t dots = 0;
  for (char c : rep) {
    if (c == '.') {
      dots += 1;
    }
  }
  if (dots > 1) {
    return nullptr;
  }
  if (dots > 0) {
    if (rep.back() == '.') {
      return nullptr;
    }
    if (rep[0] == '.') {
      return nullptr;
    }
    return s_node(new json_node_number(atof(rep.c_str())));
  } else {
    return s_node(new json_node_number(atoi(rep.c_str())));
  }
}

s_node parse_bool(json_parse_context &ctx) {
  ctx.skipWS();
  if (!ctx.ok()) {
    return nullptr;
  }
  std::string rep;
  while (ctx.ok() && (ctx.c() >= 'a' && ctx.c() <= 'z')) {
    rep += ctx.c();
    ctx.next();
  }
  if (rep == "true") {
    return s_node(new json_node_bool(true));
  } else if (rep == "false") {
    return s_node(new json_node_bool(false));
  } else {
    return nullptr;
  }
}

s_node parse_null(json_parse_context &ctx) {
  ctx.skipWS();
  if (!ctx.ok()) {
    return nullptr;
  }
  std::string rep;
  while (ctx.ok() && (ctx.c() >= 'a' && ctx.c() <= 'z')) {
    rep += ctx.c();
    ctx.next();
  }
  if (rep == "null") {
    return s_node(new json_node_null);
  } else {
    return nullptr;
  }
}

s_node parse_any(json_parse_context &ctx) {
  ctx.skipWS();
  if (!ctx.ok()) {
    return nullptr;
  }
  if (ctx.c() == '{') {
    return parse_object(ctx);
  } else if (ctx.c() == '\"') {
    return parse_string(ctx);
  } else if ((ctx.c() >= '0' && ctx.c() <= '9') || ctx.c() == '.') {
    return parse_number(ctx);
  } else if (ctx.c() == '[') {
    return parse_array(ctx);
  } else if (ctx.c() == 't' || ctx.c() == 'f') {
    return parse_bool(ctx);
  } else {
    return parse_null(ctx);
  }
  return nullptr;
}
