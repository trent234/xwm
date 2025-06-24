# Display source context
set pagination off
set print pretty on
set print elements 0        # Show all elements of arrays
set print null-stop on      # Stop strings at NULL

# Enable line numbers in backtraces
set backtrace limit 50

# Show register values on stop
set verbose on
set history save on
set history size 1000

# Automatically load symbols for shared libraries
set auto-solib-add on

# Set default breakpoint pending behavior
set breakpoint pending on

# Add source directories (can add multiple)
dir src
dir /home/user/code/xwm

# Automatically run a command on start to-
# 1. Set a breakpoint at main
# 2. Immediately start the program (run)
# 3. Pause at the first line of user code
break main
run
