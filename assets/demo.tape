# Visual Settings
Output fswap-demo.gif
Require fswap
Set FontSize 17
Set Width 1000
Set Height 800
Set Theme "Catppuccin Mocha"
Set Padding 20
Set TypingSpeed 0.08s

# Hidden setup
Hide
Type "echo 'production_db_pass=secret' > config.json" Enter
Type "echo 'test_db_pass=1234' > config.json.bak" Enter
Type "chmod 600 config.json" Enter
Type "chmod 644 config.json.bak" Enter
Type "mkdir -p app app.bak" Enter
Type "echo 'RELEASE_MODE' > app/build.txt" Enter
Type "echo 'DEBUG_MODE' > app.bak/build.txt" Enter
Type "chmod 755 app" Enter
Type "chmod 700 app.bak" Enter
Type "clear" Enter
Show

# Scene 1: Initial state
Type "# 1. Initial state: config.json is 600, .bak is 644" Enter
Sleep 500ms
Type "ls -li config.json*" Enter
Sleep 1.5s
Type "cat config.json" Enter
Sleep 2s
Enter

# Scene 2: Default Swap (Content only, keeps permissions)
Type "# 2. Default swap: changes content, KEEPS permissions and inodes intact" Enter
Sleep 500ms
Type "fswap -v config.json config.json.bak" Enter
Sleep 2s
Type "ls -li config.json*" Enter
Sleep 1.5s
Type "cat config.json" Enter
Sleep 3s
Type "clear" Enter

# Scene 3: Undo
Type "# 3. Made a mistake? Instantly undo!" Enter
Sleep 500ms
Type "fswap -uv" Enter
Sleep 2s
Type "ls -li config.json*" Enter
Sleep 2s
Enter

# Scene 4: Atomic Swap (-a)
Type "# 4. Need to swap huge files? Use -a for instant kernel-level swap (swaps inodes)" Enter
Sleep 500ms
Type "fswap -av config.json config.json.bak" Enter
Sleep 2s
Type "ls -li config.json*" Enter
Sleep 3s
Type "clear" Enter

# Scene 5: Directory Swap
Type "# 5. Bonus: Swaps whole directory contents while keeping folder permissions!" Enter
Sleep 500ms
Type "ls -ld app app.bak" Enter
Sleep 1.5s
Type "# Let's see what's inside before swapping" Enter
Sleep 500ms
Type "cat app/build.txt && cat app.bak/build.txt" Enter
Sleep 2s
Type "fswap -v app app.bak" Enter
Sleep 2s
Type "# The permissions of the folders are UNCHANGED..." Enter
Sleep 500ms
Type "ls -ld app app.bak" Enter
Sleep 1.5s
Type "# ...but the contents inside have been swapped!" Enter
Sleep 500ms
Type "cat app/build.txt && cat app.bak/build.txt" Enter
Sleep 4s

# Scene 6: Directory Swap with Permissions (-p)
Type "# 6. Wait, want to swap the folder permissions too? Just add -p!" Enter
Sleep 500ms
Type "fswap -vp app app.bak" Enter
Sleep 2s
Type "# Boom! Contents AND permissions are swapped." Enter
Sleep 500ms
Type "ls -ld app app.bak" Enter
Sleep 4s
