#pragma once

#if HSH_PROFILE_MODE

#include <iostream>
#include <sstream>
#include <map>

namespace hsh {
struct value_formatter {
  template <typename T>
  static std::ostream &format(std::ostream &out, T val) noexcept {
    return out << val;
  }
};

class profiler {
  friend class profile_context;
  const char *source = nullptr;

public:
  struct push {
    const char *name;
    explicit push(const char *name) noexcept : name(name) {}
  };
  struct pop {};
  struct cast_base {};
  template <typename T> struct cast : cast_base {
    const char *type;
    T val;
    explicit cast(const char *type, T val) noexcept : type(type), val(val) {}
  };

private:
  template <typename T>
  using EnableIfNonControlArg =
      std::enable_if_t<!std::is_same_v<T, push> && !std::is_same_v<T, pop> &&
                           !std::is_same_v<T, const char *> &&
                           !std::is_base_of_v<cast_base, T>,
                       int>;
  struct node {
    std::map<std::string, node> children;
    std::string leaf;
    node &get() noexcept { return *this; }
    template <typename... Args> node &get(push, Args... rest) noexcept {
      return get(rest...);
    }
    template <typename... Args> node &get(pop, Args... rest) noexcept {
      return get(rest...);
    }
    template <typename... Args>
    node &get(const char *arg, Args... rest) noexcept {
      return get(rest...);
    }
    template <typename T, typename... Args>
    node &get(cast<T> arg, Args... rest) noexcept {
      std::ostringstream ss;
      hsh::value_formatter::format(ss, arg.val);
      return children[ss.str()].get(rest...);
    }
    template <typename T, typename... Args, EnableIfNonControlArg<T> = 0>
    node &get(T arg, Args... rest) noexcept {
      std::ostringstream ss;
      hsh::value_formatter::format(ss, arg);
      return children[ss.str()].get(rest...);
    }
    void write(std::ostream &out, const char *src) const
        noexcept {
      if (!children.empty()) {
        for (auto [key, node] : children)
          node.write(out, src);
      } else {
        out << "template " << src << leaf << ";\n";
      }
    }
  } root;
  static void do_format_param(std::ostream &out, push p) noexcept {
    out << p.name << "<";
  }
  static void do_format_param(std::ostream &out, pop p) noexcept { out << ">"; }
  static void do_format_param(std::ostream &out, const char *arg) noexcept {
    out << arg;
  }
  template <typename T>
  static void do_format_param(std::ostream &out, cast<T> arg) noexcept {
    out << "static_cast<" << arg.type << ">(";
    hsh::value_formatter::format(out, arg.val);
    out << ')';
  }
  template <typename T, EnableIfNonControlArg<T> = 0>
  static void do_format_param(std::ostream &out, T arg) noexcept {
    hsh::value_formatter::format(out, arg);
  }
  static void format_param_next(std::ostream &out, bool &needs_comma) noexcept {
  }
  template <typename T>
  static void format_param_next(std::ostream &out, bool &needs_comma,
                                T arg) noexcept {
    if (!std::is_same_v<T, pop> && needs_comma)
      out << ", ";
    else
      needs_comma = true;
    do_format_param(out, arg);
    if constexpr (std::is_same_v<T, push>)
      needs_comma = false;
  }
  template <typename... Args>
  static void format_params(std::ostream &out, Args... args) noexcept {
    bool needs_comma = false;
    (format_param_next(out, needs_comma, args), ...);
  }
  void write_header(std::ostream &out) const noexcept {
    root.write(out, source);
  }

public:
  template <typename... Args> void add(Args... args) noexcept {
    node &n = root.get(args...);
    std::ostringstream ss;
    ss << '<';
    format_params(ss, args...);
    ss << '>';
    n.leaf = ss.str();
  }
};

class profile_context {
  struct File {
    std::map<std::string, profiler> profilers;
  };
  std::map<std::string, File> files;

public:
  static profile_context instance;
  profiler &get(const char *filename, const char *binding,
                const char *source) noexcept {
    auto &file = files[filename];
    auto &ret = file.profilers[binding];
    ret.source = source;
    return ret;
  }
  void write_headers() noexcept {
    for (auto &[filename, file] : files) {
      std::ofstream out(filename);
      if (!out.is_open()) {
        std::cerr << "Unable to open '" << filename << "' for writing\n";
        continue;
      }
      for (auto &[binding, prof] : file.profilers) {
        out << "// " << binding << "\n";
        prof.write_header(out);
      }
    }
  }
  ~profile_context() noexcept { write_headers(); }
};

inline profile_context profile_context::instance{};
} // namespace hsh

#endif
