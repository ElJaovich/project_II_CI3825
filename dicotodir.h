#ifndef DICOTODIR_H
#define DICOTODIR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <jansson.h>
#include <stdbool.h>

// Estructura para almacenar las opciones del programa
typedef struct {
    char *root_dir;
    char *true_text;
    char *false_text;
    bool prefix_mode;
    bool suffix_mode;
} Options;

// Prototipos de funciones
Options parse_options(int argc, char *argv[]);
void create_directory_structure(json_t *root, const char *base_path, Options options);
void process_species(json_t *species, const char *base_path, Options options);
void create_species_file(const char *path, const char *species_name);
char *build_dir_name(const char *question, bool answer, Options options);
void print_usage();

#endif // DICOTODIR_H