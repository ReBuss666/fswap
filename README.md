
# fswap

`fswap` is a fast, lightweight command-line utility for Linux that swaps the contents or locations of two files.

Unlike traditional `mv` hacks, `fswap` defaults to swapping **only file contents**, preserving original file inodes, paths, ownership, and `chmod` permissions. It also supports atomic filesystem-level swaps via the Linux kernel's `renameat2(RENAME_EXCHANGE)` system call.

## Features

- **Safe by Default:** Swaps file contents while keeping original file permissions and paths intact.
- **Atomic Swapping (`-a`):** Instant exchange using kernel `renameat2(RENAME_EXCHANGE)` for files of any size (with automatic cross-device fallback).
- **Permission Exchange (`-p`):** Swap `chmod` permissions along with file contents.
- **Undo Support (`-u`):** Single-level undo for your last swap operation.
- **Combined Flags:** Full support for short flag chaining (e.g., `fswap -avp`).

## Installation

### From Source

```bash
git clone https://github.com/ReBuss666/fswap.git
cd fswap
make
sudo make install

```

### Arch Linux (Local PKGBUILD)

```bash
git clone https://github.com/ReBuss666/fswap.git
cd fswap
makepkg -si

```

## Usage

```bash
fswap [FLAGS] <file1> <file2>
fswap -u

```

### Flags

| Flag | Long Flag | Description |
| --- | --- | --- |
| `-a` | `--atomic` | Use kernel RENAME_EXCHANGE (swaps whole files together with permissions). |
| `-p` | `--perms` | Swap chmod permissions along with file contents. |
| `-u` | `--undo` | Revert the last swap operation. |
| `-v` | `--verbose` | Print detailed execution log. |
| `-h` | `--help` | Display help menu. |

### Examples

```bash
# Default content swap (permissions stay on their respective paths)
fswap config.json config.json.bak

# Instant atomic swap for large files (e.g. ISOs or database dumps)
fswap -a large_v1.iso large_v2.iso

# Swap content AND chmod permissions
fswap -ap script.sh script.old.sh

# Undo last swap operation
fswap -u

```

## License

MIT

```
