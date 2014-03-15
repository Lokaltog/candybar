#include <errno.h>
#include "wkline.h"

json_t* get_config_json (char *config_override_filename);
json_t* get_config_option (json_t *config_object, const char *key, bool silent);

#define get_config_option_string(OBJ, OPTION)  json_string_value(get_config_option(OBJ, OPTION, false))
#define get_config_option_integer(OBJ, OPTION) json_integer_value(get_config_option(OBJ, OPTION, false))
#define get_config_option_real(OBJ, OPTION)    json_real_value(get_config_option(OBJ, OPTION, false))
#define get_config_option_boolean(OBJ, OPTION) json_is_true(get_config_option(OBJ, OPTION, false))

#define get_config_option_string_silent(OBJ, OPTION)  json_string_value(get_config_option(OBJ, OPTION, true))
#define get_config_option_integer_silent(OBJ, OPTION) json_integer_value(get_config_option(OBJ, OPTION, true))
#define get_config_option_real_silent(OBJ, OPTION)    json_real_value(get_config_option(OBJ, OPTION, true))
#define get_config_option_boolean_silent(OBJ, OPTION) json_is_true(get_config_option(OBJ, OPTION, true))
