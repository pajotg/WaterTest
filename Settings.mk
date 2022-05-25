NAME = WaterTest

INCLUDE_DIRS = src/ MLX42/include/MLX42

CFLAGS += -O3				# I NEED SPEEEED
#CFLAGS += -O0 -g3 -ggdb		# for debugging purposes
CFLAGS += -fsanitize=address
#CFLAGS += -D NDEBUG		# remove assert calls
CFLAGS += -ferror-limit=2	# usually only the first or second errors are usefull, the rest is just junk

CFLAGS += -Wno-newline-eof
CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter

LDFLAGS += -lglfw -L "/Users/$(USER)/.brew/opt/glfw/lib/" MLX42/libmlx42.a -L MLX42 

ifdef RUST
LDFLAGS += rust_export/target/release/librust_export.dylib
CFLAGS += -D RUST
endif