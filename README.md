## Debug build
CC=clang meson setup build-debug -Dc_link_args="-fsanitize=address" -Dc_args="-fsanitize=address"
