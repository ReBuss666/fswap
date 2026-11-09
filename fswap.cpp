#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE (1 << 1)
#endif

// Уникальный путь вида /var/tmp/fswap_state_1000
std::string get_state_file_path() {
  return "/var/tmp/fswap_state_" + std::to_string(getuid());
}

int sys_renameat2(int olddirfd, const char *oldpath, int newdirfd,
                  const char *newpath, unsigned int flags) {
  return syscall(SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
}

struct Options {
  bool atomic = false;     // -a
  bool swap_perms = false; // -p
  bool verbose = false;    // -v
  bool help = false;       // -h
  bool undo = false;       // -u
  std::vector<fs::path> files;
};

void print_help(const char *prog_name) {
  std::cout
      << "Usage: " << prog_name << " [FLAGS] <path1> <path2>\n"
      << "       " << prog_name << " -u\n\n"
      << "Swaps two files or two directories (both must be of the same "
         "type).\n"
      << "By default, only contents are swapped. Permissions of the files/\n"
      << "directories themselves remain unchanged.\n\n"
      << "For directories, the whole tree inside is swapped recursively.\n\n"
      << "Flags:\n"
      << "  -a, --atomic    Swap entries entirely using RENAME_EXCHANGE\n"
      << "                  (permissions and ownership move together with "
         "them).\n"
      << "                  Works for files and directories. If the paths are "
         "on\n"
      << "                  different devices, falls back to content swap.\n"
      << "  -p, --perms     Swap permissions as well (used with default "
         "content swap).\n"
      << "  -u, --undo      Undo the last swap operation.\n"
      << "  -v, --verbose   Show detailed output.\n"
      << "  -h, --help      Show this help message.\n\n"
      << "Examples:\n"
      << "  " << prog_name << " conf conf.bak            # swap two files\n"
      << "  " << prog_name
      << " dir1 dir2                # swap two "
         "directories\n"
      << "  " << prog_name << " -a file1 file2           # atomic swap\n"
      << "  " << prog_name << " -ap script.sh script.old.sh\n"
      << "  " << prog_name << " -u                       # undo last swap\n";
}

Options parse_arguments(int argc, char *argv[]) {
  Options opts;
  bool no_more_flags = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (!no_more_flags && arg == "--") {
      no_more_flags = true;
    } else if (!no_more_flags && arg.length() > 1 && arg[0] == '-') {
      if (arg == "--atomic")
        opts.atomic = true;
      else if (arg == "--perms")
        opts.swap_perms = true;
      else if (arg == "--undo")
        opts.undo = true;
      else if (arg == "--verbose")
        opts.verbose = true;
      else if (arg == "--help")
        opts.help = true;
      else if (arg[1] != '-') {
        for (size_t j = 1; j < arg.length(); ++j) {
          switch (arg[j]) {
          case 'a':
            opts.atomic = true;
            break;
          case 'p':
            opts.swap_perms = true;
            break;
          case 'u':
            opts.undo = true;
            break;
          case 'v':
            opts.verbose = true;
            break;
          case 'h':
            opts.help = true;
            break;
          default:
            std::cerr << "Unknown flag: -" << arg[j] << "\n";
            exit(1);
          }
        }
      } else {
        std::cerr << "Unknown flag: " << arg << "\n";
        exit(1);
      }
    } else {
      opts.files.push_back(arg);
    }
  }
  return opts;
}

void save_state(const Options &opts, const fs::path &f1, const fs::path &f2) {
  std::string state_file = get_state_file_path();
  std::ofstream out(state_file, std::ios::trunc);
  if (out) {
    out << (opts.atomic ? "atomic" : (opts.swap_perms ? "perms" : "content"))
        << "\n"
        << fs::absolute(f1).string() << "\n"
        << fs::absolute(f2).string() << "\n";
  }
}

bool load_state(Options &opts) {
  std::string state_file = get_state_file_path();
  std::ifstream in(state_file);
  if (!in)
    return false;

  std::string mode, p1, p2;
  if (std::getline(in, mode) && std::getline(in, p1) && std::getline(in, p2)) {
    opts.atomic = (mode == "atomic");
    opts.swap_perms = (mode == "perms");
    opts.files = {fs::path(p1), fs::path(p2)};
    return true;
  }
  return false;
}

bool copy_content(const fs::path &src, const fs::path &dst) {
  std::ifstream in(src, std::ios::binary);
  std::ofstream out(dst, std::ios::binary | std::ios::trunc);

  if (!in || !out)
    return false;

  constexpr size_t BUFFER_SIZE = 8 * 1024 * 1024; // 8 MB buffer
  std::vector<char> buffer(BUFFER_SIZE);

  while (in.read(buffer.data(), buffer.size())) {
    out.write(buffer.data(), in.gcount());
  }
  out.write(buffer.data(), in.gcount());

  return static_cast<bool>(out);
}

// Удаляет всё содержимое папки, саму папку оставляет
void clear_dir(const fs::path &dir) {
  for (const auto &entry : fs::directory_iterator(dir))
    fs::remove_all(entry.path());
}

// Копирует содержимое папки src внутрь уже существующей папки dst
void copy_dir_into(const fs::path &src, const fs::path &dst) {
  fs::copy(src, dst,
           fs::copy_options::recursive | fs::copy_options::copy_symlinks |
               fs::copy_options::overwrite_existing);
}

// Обмен содержимым двух папок (сами папки и их права остаются на месте)
void swap_dir_contents(const fs::path &d1, const fs::path &d2,
                       const fs::path &temp) {
  fs::copy(d1, temp,
           fs::copy_options::recursive | fs::copy_options::copy_symlinks);
  try {
    clear_dir(d1);
    copy_dir_into(d2, d1);
    clear_dir(d2);
    copy_dir_into(temp, d2);
  } catch (...) {
    std::cerr << "Operation failed midway. Backup of " << d1
              << " kept at: " << temp << "\n";
    throw;
  }
  fs::remove_all(temp);
}

// Убирает завершающий слэш: "dir/" -> "dir"
fs::path normalize(const fs::path &p) {
  fs::path n = p.lexically_normal();
  if (!n.has_filename() && n.has_parent_path())
    n = n.parent_path();
  return n;
}

bool is_inside(const fs::path &child, const fs::path &parent) {
  fs::path c = fs::weakly_canonical(child);
  fs::path p = fs::weakly_canonical(parent);
  auto [pe, ce] = std::mismatch(p.begin(), p.end(), c.begin(), c.end());
  return pe == p.end();
}

bool is_supported(const fs::path &p) {
  return fs::exists(p) && (fs::is_regular_file(p) || fs::is_directory(p));
}

int main(int argc, char *argv[]) {
  Options opts = parse_arguments(argc, argv);

  if (opts.help) {
    print_help(argv[0]);
    return 0;
  }

  if (opts.undo) {
    if (!load_state(opts)) {
      std::cerr << "Error: No previous swap to undo.\n";
      return 1;
    }
    if (opts.verbose)
      std::cout << "[Info] Undoing last swap...\n";
  } else if (opts.files.size() != 2) {
    std::cerr << "Error: You must specify exactly two paths.\n"
              << "Try '" << argv[0] << " -h' for help.\n";
    return 1;
  }

  fs::path file1 = normalize(opts.files[0]);
  fs::path file2 = normalize(opts.files[1]);

  for (const auto &p : {file1, file2}) {
    if (!is_supported(p)) {
      std::cerr << "Error: " << p
                << " does not exist or is not a regular file/directory.\n";
      return 1;
    }
  }

  bool is_dir1 = fs::is_directory(file1);
  bool is_dir2 = fs::is_directory(file2);
  if (is_dir1 != is_dir2) {
    std::cerr << "Error: Cannot swap a file with a directory.\n";
    return 1;
  }

  std::error_code ec;
  if (fs::equivalent(file1, file2, ec)) {
    std::cerr << "Error: Both paths refer to the same object.\n";
    return 1;
  }

  if (is_dir1 && (is_inside(file1, file2) || is_inside(file2, file1))) {
    std::cerr << "Error: One directory is inside the other.\n";
    return 1;
  }

  try {
    if (opts.atomic) {
      if (opts.verbose)
        std::cout << "[Info] Executing atomic swap...\n";

      if (sys_renameat2(AT_FDCWD, file1.c_str(), AT_FDCWD, file2.c_str(),
                        RENAME_EXCHANGE) == 0) {
        if (opts.verbose)
          std::cout << "✅ Atomic swap successful.\n";
      } else {
        if (errno == EXDEV) {
          if (opts.verbose)
            std::cout << "[Warning] Cross-device link. Falling back to content "
                         "swap.\n";
          opts.atomic = false;
        } else {
          perror("renameat2 failed");
          return 1;
        }
      }
    }

    if (!opts.atomic) {
      if (opts.verbose)
        std::cout << "[Info] Swapping contents...\n";

      fs::path temp =
          file1.string() + ".swap." + std::to_string(getpid()) + ".tmp";

      if (is_dir1) {
        swap_dir_contents(file1, file2, temp);
      } else {
        if (!copy_content(file1, temp))
          throw std::runtime_error("Failed to read " + file1.string());
        if (!copy_content(file2, file1))
          throw std::runtime_error("Failed to write to " + file1.string());
        if (!copy_content(temp, file2))
          throw std::runtime_error("Failed to write to " + file2.string());

        fs::remove(temp);
      }

      if (opts.swap_perms) {
        if (opts.verbose)
          std::cout << "[Info] Swapping permissions...\n";
        auto perms1 = fs::status(file1).permissions();
        auto perms2 = fs::status(file2).permissions();
        fs::permissions(file1, perms2, fs::perm_options::replace);
        fs::permissions(file2, perms1, fs::perm_options::replace);
      }

      if (opts.verbose)
        std::cout << "✅ Content swap successful.\n";
    }

    if (!opts.undo) {
      save_state(opts, file1, file2);
    } else {
      fs::remove(get_state_file_path());
      if (opts.verbose)
        std::cout << "✅ Undo completed. State cleared.\n";
    }

  } catch (const std::exception &e) {
    std::cerr << "Critical Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
