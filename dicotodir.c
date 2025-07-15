#include "dicotodir.h"
#include <libgen.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    // Parsear opciones
    Options options = parse_options(argc, argv);

    // Verificar que se proporcionó un archivo JSON
    if (optind >= argc) {
        fprintf(stderr, "Error: Se requiere un archivo JSON como argumento.\n");
        print_usage();
        return EXIT_FAILURE;
    }

    const char *json_file = argv[optind];

    // Leer y parsear el archivo JSON
    json_error_t error;
    json_t *root = json_load_file(json_file, 0, &error);

    if (!root) {
        fprintf(stderr, "Error: No se pudo leer el archivo JSON (línea %d, columna %d): %s\n",
                error.line, error.column, error.text);
        return EXIT_FAILURE;
    }

    // Crear directorio raíz si no existe
    if (mkdir(options.root_dir, 0755) != 0 && errno != EEXIST) {
        perror("Error al crear directorio raíz");
        json_decref(root);
        return EXIT_FAILURE;
    }

    // Procesar la estructura JSON
    create_directory_structure(root, options.root_dir, options);

    // Liberar memoria
    json_decref(root);
    free(options.root_dir);
    free(options.true_text);
    free(options.false_text);

    return EXIT_SUCCESS;
}

Options parse_options(int argc, char *argv[]) {
    Options options = {
        .root_dir = strdup("."),
        .true_text = strdup("si tiene"),
        .false_text = strdup("no tiene"),
        .prefix_mode = true,
        .suffix_mode = false
    };

    int opt;
    while ((opt = getopt(argc, argv, "d:t:f:ps")) != -1) {
        switch (opt) {
            case 'd':
                free(options.root_dir);
                options.root_dir = strdup(optarg);
                break;
            case 't':
                free(options.true_text);
                options.true_text = strdup(optarg);
                break;
            case 'f':
                free(options.false_text);
                options.false_text = strdup(optarg);
                break;
            case 'p':
                options.prefix_mode = true;
                options.suffix_mode = false;
                break;
            case 's':
                options.suffix_mode = true;
                options.prefix_mode = false;
                break;
            default:
                print_usage();
                exit(EXIT_FAILURE);
        }
    }

    return options;
}

void create_directory_structure(json_t *root, const char *base_path, Options options) {
    const char *key;
    json_t *value;

    json_object_foreach(root, key, value) {
        if (!json_is_array(value)) {
            fprintf(stderr, "Warning: Valor inesperado para la clave '%s'. Se esperaba un array.\n", key);
            continue;
        }

        size_t index;
        json_t *species_obj;
        json_array_foreach(value, index, species_obj) {
            process_species(species_obj, base_path, options);
        }
    }
}

void process_species(json_t *species, const char *base_path, Options options) {
    const char *species_name;
    json_t *questions;

    json_object_foreach(species, species_name, questions) {
        if (!json_is_array(questions)) {
            fprintf(stderr, "Warning: Valor inesperado para la especie '%s'. Se esperaba un array.\n", species_name);
            continue;
        }

        char *current_path = strdup(base_path);
        size_t q_index;
        json_t *question_obj;

        json_array_foreach(questions, q_index, question_obj) {
            const char *question;
            json_t *answer;

            json_object_foreach(question_obj, question, answer) {
                if (!json_is_boolean(answer)) {
                    fprintf(stderr, "Warning: Respuesta inesperada para la pregunta '%s'. Se esperaba un booleano.\n", question);
                    continue;
                }

                bool ans = json_boolean_value(answer);
                char *dir_name = build_dir_name(question, ans, options);

                // Construir la nueva ruta
                char *new_path;
                asprintf(&new_path, "%s/%s", current_path, dir_name);

                // Crear directorio si no existe
                if (mkdir(new_path, 0755) != 0 && errno != EEXIST) {
                    perror("Error al crear directorio");
                    free(dir_name);
                    free(new_path);
                    free(current_path);
                    return;
                }

                // Actualizar la ruta actual
                free(current_path);
                current_path = new_path;
                free(dir_name);
            }
        }

        // Crear archivo de la especie
        create_species_file(current_path, species_name);
        free(current_path);
    }
}

void create_species_file(const char *path, const char *species_name) {
    char *file_path;
    asprintf(&file_path, "%s/%s.txt", path, species_name);

    FILE *file = fopen(file_path, "w");
    if (!file) {
        perror("Error al crear archivo de especie");
        free(file_path);
        return;
    }

    fclose(file);
    free(file_path);
}

char *build_dir_name(const char *question, bool answer, Options options) {
    const char *text = answer ? options.true_text : options.false_text;
    char *dir_name;

    if (options.prefix_mode) {
        asprintf(&dir_name, "%s %s", text, question);
    } else {
        asprintf(&dir_name, "%s %s", question, text);
    }

    return dir_name;
}

void print_usage() {
    fprintf(stderr, "Uso: ./dicotodir <clave> [opciones]\n");
    fprintf(stderr, "Opciones:\n");
    fprintf(stderr, "  -d, --dir <raiz>    Directorio raíz para crear la estructura (por defecto: .)\n");
    fprintf(stderr, "  -t, --true <texto>  Texto para respuestas verdaderas (por defecto: 'si tiene')\n");
    fprintf(stderr, "  -f, --false <texto> Texto para respuestas falsas (por defecto: 'no tiene')\n");
    fprintf(stderr, "  -p, --pre           Usar texto como prefijo (por defecto)\n");
    fprintf(stderr, "  -s, --suf           Usar texto como sufijo (desactiva --pre)\n");
}