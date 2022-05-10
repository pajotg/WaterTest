# things that can be overridden by Settings.mk
NAME = a.out

SRC_DIR = src/
OBJ_DIR = obj/
PREREQ_DIR = prereq/
INCLUDE_DIRS = include/

CFLAGS = -Wall -Wextra -Werror -std=c++17 -pedantic-errors -DCOMPILE
LDFLAGS = 
CXX = clang++

sinclude Settings.mk

SOURCE_FILES = $(subst $(SRC_DIR)/,$(SRC_DIR),$(shell find $(SRC_DIR) -type f -name *.cpp))

INCLUDE_DIRS := $(shell find $(INCLUDE_DIRS) -type d)

OBJECTS = $(SOURCE_FILES:$(SRC_DIR)%.cpp=$(OBJ_DIR)%.o)
PREREQS = $(SOURCE_FILES:$(SRC_DIR)%.cpp=$(PREREQ_DIR)%.d)

CFLAGS += $(INCLUDE_DIRS:%=-I%)

.PHONY: all
all: $(NAME)

sinclude $(PREREQS)

$(NAME): $(OBJECTS) Makefile Settings.mk | $(OBJ_DIR)
	@echo "Making $@"
	$(CXX) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJECTS): Makefile Settings.mk | $(SRC_DIR) $(OBJ_DIR)
	@echo "Making $@"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CFLAGS) -c -o $@ $(@:$(OBJ_DIR)%.o=$(SRC_DIR)%.cpp)

$(OBJ_DIR) $(SRC_DIR) $(PREREQ_DIR):
	@echo "Making $@"
	@mkdir $@

# magic includes

# make sure to delete the corrupted file in case of a error, since we will include that
# and if its corrupted, the makefile will be "corrupted"

.DELETE_ON_ERROR: $(PREREQS)
$(PREREQS): $(PREREQ_DIR)%.d: $(SRC_DIR)%.cpp Makefile Settings.mk | $(PREREQ_DIR)
	@echo "Making $@"
	@mkdir -p $(shell dirname $@)
	@printf "$@ $(shell dirname $(patsubst $(PREREQ_DIR)%,$(OBJ_DIR)%,$@))/" > $@
	@$(CXX) -MM $(patsubst $(PREREQ_DIR)%.d,$(SRC_DIR)%.cpp,$@) $(CFLAGS) >> $@

# general management
.PHONY: pclean
pclean:
	rm -rf $(PREREQ_DIR)

.PHONY: clean
clean: pclean
	rm -rf $(OBJ_DIR)

.PHONY: fclean
fclean: clean
	rm -f $(NAME)

.PHONY: re
re: | fclean all
