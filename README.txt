##########################################################################################################################
# RC PROJECT 2022/2023 | INSTITUTO SUPERIOR TÉCNICO | LEIC-A | MADE BY: PAULO ALMEIDA ist98959 -- IVAN FORTES ist99085   #                                                                                                                             #
##########################################################################################################################


        --> To run the program, you must have C installed on your computer (preferably the most recent version). <--



1º- Run 'make' in the 'hangman-rc' folder to compile the program. This will create the executable files 'GC' and 'player'.

2º- Go to the 'server' folder, and run './GC wordfile.txt [-p] [-v] [-s]'.
    [-p] is an optional argument that allows you to give another port besides the default port (58079). Use: '-p port_number'.
    [-v] is an optional argument that allows you to see extra information about the socket connections. Use '-v'.
    [-s] is an optional argument that allows you to switch to a sequential choice of words from the word file. Use '-s'.

3º- Go to the 'client' folder, and run './player [-n] [-p]'.
    [-n] is an optional argument that allows you to access a server with the given public ip. Use: '-n public_ip'.
    [-p] is an optional argument that allows you to give another port besides the default port (58079). Use: '-p port_number'.
    
4º- Use 'make clean' to delete the executable files, score files, game files, and recent files.

5º- Have fun!

##########################################################################################################################

# CONSIDERATIONS FOR THE GAME:
     
     - The 'REV' command is not implemented.
     - The word file used is 'word_eng.txt', and it contains 26 words.
     - Use the -s command after the -v command, if you want to use both.
     - The hint images are stored in the 'HINTS' folder, located in the 'server' folder.
     - The 'GAMES' and 'SCORES' folders are already created in the 'server' folder. 
     - The '-s' flag was created to pass the player scripts. Use if needed.
     - The client application does extra input validation, such as starting a command with no PLID or with a PLID that does not exist.
     - The client application sockets have a timeout of 10 seconds.
     - The score/hint/word/state files have a different timestamp and format from the one suggested by the server specification file.

     
