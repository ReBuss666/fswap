# fswap

`fswap` is a fast, lightweight command-line utility written in C++ for Linux that swaps the contents, trees, or locations of two files or directories.

Unlike traditional `mv` hacks, `fswap` defaults to swapping **only contents**, preserving original file inodes, paths, ownership, and `chmod` permissions. For directories, it recursively swaps everything inside them while keeping the top-level directories and their attributes in place. It also supports atomic filesystem-level swaps via the Linux kernel's `renameat2(RENAME_EXCHANGE)` system call.

<div align="center">
  <img src="assets/fswap-demo.gif" alt="fswap demo in terminal" width="800">
</div>

## Features

- **Files & Directories:** Works seamlessly with both regular files and entire directory trees recursively.
- **Safe by Default:** Swaps contents while keeping original paths, inodes, and permissions intact.
- **Atomic Swapping (`-a`):** Instant exchange using kernel `renameat2(RENAME_EXCHANGE)`. Works for both files and directories on the same filesystem (with automatic fallback to content/recursive copy if cross-device).
- **Permission Exchange (`-p`):** Swap `chmod` permissions along with content (for non-atomic mode).
- **Undo Support (`-u`):** Single-level undo (`/var/tmp/fswap_state_$UID`) to safely revert your last swap operation.
- **Combined Flags:** Full support for short flag chaining (e.g., `fswap -avp`).

## Installation

### Download Binary (Easiest)

You can download the pre-compiled binary directly from the [Releases](https://github.com/ReBuss666/fswap/releases) page.
Just download the `fswap` file, make it executable, and move it to your bin directory:

```bash
chmod +x fswap
sudo mv fswap /usr/local/bin/
```

### From Source

```bash
git clone [https://github.com/ReBuss666/fswap.git](https://github.com/ReBuss666/fswap.git)
cd fswap
make
sudo make install

```

### Arch Linux (Local PKGBUILD)

```bash
git clone [https://github.com/ReBuss666/fswap.git](https://github.com/ReBuss666/fswap.git)
cd fswap
makepkg -si

```

## Usage

```bash
fswap [FLAGS] <path1> <path2>
fswap -u

```

### Flags

| Short | Long Flag   | Description                                                                                                                                         |
| ----- | ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `-a`  | `--atomic`  | Use kernel `RENAME_EXCHANGE` (swaps directory entries entirely, including metadata/permissions). Falls back to standard swap on cross-device links. |
| `-p`  | `--perms`   | Swap `chmod` permissions along with file contents (used in default mode).                                                                           |
| `-u`  | `--undo`    | Revert the last swap operation using the saved state.                                                                                               |
| `-v`  | `--verbose` | Print detailed execution logs.                                                                                                                      |
| `-h`  | `--help`    | Display help menu.                                                                                                                                  |

### Examples

```bash
# Default content swap (permissions stay on their original paths)
fswap config.json config.json.bak

# Swap two directories recursively (contents inside are swapped)
fswap dir_v1 dir_v2

# Instant atomic swap for large files or directories (same filesystem)
fswap -a large_v1.iso large_v2.iso

# Swap content AND permissions for scripts
fswap -ap script1.sh script2.sh

# Verbose atomic swap with detailed output
fswap -av /var/www/site_v1 /var/www/site_v2

# Undo the last operation if something went wrong
fswap -u

```

## How It Works

1. **Default Mode (Content Swap):**

- For files: Safely copies data using an 8MB buffer via a temporary file (`.swap.<PID>.tmp`).
- For directories: Recursively copies `dir1` to a temporary backup, clears `dir1`, copies `dir2` into `dir1`, clears `dir2`, and moves the backup into `dir2`.

1. **Atomic Mode (`-a`):** Calls `syscall(SYS_renameat2, ..., RENAME_EXCHANGE)` which swaps the paths at the filesystem inode level atomically and instantaneously.
2. **Undo State:** Every successful swap operation saves the absolute paths and mode to `/var/tmp/fswap_state_$UID`, allowing a quick rollback via `fswap -u`.

## License

MIT
