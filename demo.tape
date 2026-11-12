# Visual Settings
Output fswap-demo.gif
Require fswap
Set FontSize 18
Set Width 1000
Set Height 600
Set Theme "Catppuccin Mocha"
Set Padding 20
Set TypingSpeed 0.08s

# Hidden setup
Hide
Type "echo 'production_db_pass=secret' > config.json" Enter
Type "echo 'test_db_pass=1234' > config.json.bak" Enter
Type "chmod 600 config.json" Enter
Type "chmod 644 config.json.bak" Enter
Type "clear" Enter
Show

# Scene 1: Initial state
Type "# 1. Let's check the initial files (note the permissions and inodes)" Enter
Sleep 500ms
Type "ls -li config.json*" Enter
Sleep 1.5s
Type "cat config.json" Enter
Sleep 2.5s
Type "clear" Enter

# Scene 2: Atomic Swap
Type "# 2. Swap them atomically using the kernel's RENAME_EXCHANGE" Enter
Sleep 500ms
Type "fswap -av config.json config.json.bak" Enter
Sleep 3s
Type "clear" Enter

# Scene 3: Verification
Type "# 3. Verify: contents swapped, but inodes & permissions are untouched!" Enter
Sleep 500ms
Type "ls -li config.json*" Enter
Sleep 1.5s
Type "cat config.json" Enter
Sleep 2.5s
Type "clear" Enter

# Scene 4: Undo
Type "# 4. Made a mistake? Instantly undo the last swap operation" Enter
Sleep 500ms
Type "fswap -uv" Enter
Sleep 2s
Type "ls -li config.json*" Enter
Sleep 4s
