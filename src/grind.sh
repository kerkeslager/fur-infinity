valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes --error-exitcode=42 "$@"
