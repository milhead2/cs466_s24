#
# Sample .gdbinit for my build directory.
#
#   Note: You should have a global .gdbinit in your home directory 
#         that governs gdb permissions..  In mine below I allow 
#         all modifications
#
#   $ cat ~/.gdbinit
#   set auto-load safe-path /
#


#
# Define some macros
#

# reload: reset and reload the applicationi
define reload
    monitor reset halt
    load
    monitor reset init
end

# mr: Perform external make and then call the above 
#     script to reload the target
define mr
    make
    reload
end

#
# Default execution befins here
#

# connect to openocd
target extended-remote :3333
# call reload macro above
reload
# Set breakpoints on entry to main() and on the assert handler
b main
b _assert_failed

# Turn the processor loose, You may not want to automatically run this command
continue


