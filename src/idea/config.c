#include "config.h"

#include <unistd.h>

#include "main.h"
#include "parser.h"

bool fill_config_with_defaults(Config *config) {
  if (!config->hostname) {
    const unsigned int default_hostname_size = 128;
    config->hostname = malloc(default_hostname_size);
    if (!config->hostname) return false;
    if (gethostname(config->hostname, default_hostname_size) == -1) {
      free(config->hostname);
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the host name");
      return false;
    }
  }

  return true;
}

bool write_config_file(FILE *file, Config config) {
  if (fprintf(file, "hostname: %s\n", config.hostname) < 0) return false;
  return true;
}

void free_config(Config c) {
  free(c.hostname);
}

bool config_hostname(Input *input) {
  if (idea_state.config.hostname) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Hostname already provided ('%s')", idea_state.config.hostname);
    return false;
  }

  idea_state.config.hostname = next_token(input, '\0');

  if (!idea_state.config.hostname) {
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "No hostname provided");
    return false;
  }

  return true;
}

Functionality config_functionality[] = {
  { "hostname:", NULL, config_hostname, MAN("Specify the hostname of the machine", "[host name]") },
  { "--", NULL, action_do_nothing, MAN("Comment", "[comment]") },
};
unsigned int config_functionality_count = sizeof(config_functionality) / sizeof(Functionality);

bool load_config() {
  FILE *config = fopen(idea_state.config_filepath, "r");
  if (!config) {
    config = fopen(idea_state.config_filepath, "w");

    if (config) { // File did not exist
      if (!fill_config_with_defaults(&idea_state.config)) {
        fclose(config);
        APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to get the default config");
        return false;
      }

      if (!write_config_file(config, idea_state.config)) {
        fclose(config);
        APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to write to the config file '%s'", idea_state.config_filepath);
        return false;
      }

      fclose(config);
      return true;
    } else { // Error
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to open the config file '%s'", idea_state.config_filepath);
      return false;
    }
  }

  unsigned int line_nr = 0;
  String_builder sb = sb_new();
  while (sb_read_line(config, &sb)) {
    line_nr++;

    if (sb.length == 0) continue;

    Input line = {
      .input = sb.str,
      .length = sb.length,
      .cursor = 0,
    };
    char *command = next_token(&line, ' ');

    if (!command) {
      sb_clean(&sb);
      free(command);
      continue;
    }

    bool (*function)(Input *) = search_functionality_function(command, config_functionality, config_functionality_count);

    if (!function) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "%s:%d: Unrecognized command '%s'", idea_state.config_filepath, line_nr, command);
      free(command);
      sb_free(&sb);
      fclose(config);
      return false;
    }

    if (!function(&line)) {
      APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "%s:%d: An error occurred while executing the command '%s'", idea_state.config_filepath, line_nr, command);
      free(command);
      sb_free(&sb);
      fclose(config);
      return false;
    }

    free(command);
    sb_clean(&sb);
  }

  if (!fill_config_with_defaults(&idea_state.config)) {
    fclose(config);
    APPEND_TO_BACKTRACE(BACKTRACE_ERROR, "Unable to fill the config with the default values");
    return false;
  }

  sb_free(&sb);
  fclose(config);
  return true;
}
