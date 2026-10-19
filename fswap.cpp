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

// const std::string STATE_FILE = "/tmp/fswap_state";
// Функция генерирует уникальный путь вида /var/tmp/fswap_state_1000
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
  std::cout << "Usage: " << prog_name << " [FLAGS] <file1> <file2>\n"
            << "       " << prog_name << " -u\n\n"
            << "Swaps the contents of two files.\n"
            << "By default, only contents are swapped. Permissions remain "
               "unchanged.\n\n"
            << "Flags:\n"
            << "  -a, --atomic    Move files entirely using RENAME_EXCHANGE "
               "(including chmod permissions).\n"
            << "  -p, --perms     Swap chmod permissions as well (used with "
               "default content swap).\n"
            << "  -u, --undo      Undo the last swap operation.\n"
            << "  -v, --verbose   Show detailed output.\n"
            << "  -h, --help      Show this help message.\n\n"
            << "Examples:\n"
            << "  " << prog_name << " conf conf.bak\n"
            << "  " << prog_name << " -a file1 file2\n"
            << "  " << prog_name << " -ap script.sh script.old.sh\n"
            << "  " << prog_name << " -u\n";
}

Options parse_arguments(int argc, char *argv[]) {
  Options opts;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg[0] == '-' && arg.length() > 1) {
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

  return true;
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
    std::cerr << "Error: You must specify exactly two files.\n"
              << "Try '" << argv[0] << " -h' for help.\n";
    return 1;
  }

  fs::path file1 = opts.files[0];
  fs::path file2 = opts.files[1];

  if (!fs::exists(file1) || !fs::is_regular_file(file1)) {
    std::cerr << "Error: " << file1
              << " does not exist or is not a regular file.\n";
    return 1;
  }
  if (!fs::exists(file2) || !fs::is_regular_file(file2)) {
    std::cerr << "Error: " << file2
              << " does not exist or is not a regular file.\n";
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

      if (!copy_content(file1, temp))
        throw std::runtime_error("Failed to read " + file1.string());
      if (!copy_content(file2, file1))
        throw std::runtime_error("Failed to write to " + file1.string());
      if (!copy_content(temp, file2))
        throw std::runtime_error("Failed to write to " + file2.string());

      fs::remove(temp);

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
