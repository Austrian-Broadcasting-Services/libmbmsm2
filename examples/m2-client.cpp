#include <cstdio>
#include <iostream>
#include <argp.h>

#include <cstdlib>
#include <cstdarg>

#include <fstream>
#include <string>
#include <chrono>
#include <thread>

#include "spdlog/async.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/syslog_sink.h"

#include "Version.h"
#include "M2Client.h"


static void print_version(FILE *stream, struct argp_state *state);
void (*argp_program_version_hook)(FILE *, struct argp_state *) = print_version;
const char *argp_program_bug_address = "Austrian Broadcasting Services <obeca@ors.at>";
static char doc[] = "libmbmsifs M2Client demo";  // NOLINT

static struct argp_option options[] = {  // NOLINT
    {"target", 't', "IF", 0, "IP address to connect to. Default: localhost (127.0.0.1)", 0},
    {"port", 'p', "PORT", 0, "Port to connect to (default: 36443)", 0},
    {"log-level", 'l', "LEVEL", 0,
     "Log verbosity: 0 = trace, 1 = debug, 2 = info, 3 = warn, 4 = error, 5 = "
     "critical, 6 = none. Default: 2.",
     0},
    {nullptr, 0, nullptr, 0, nullptr, 0}};

/**
 * Holds all options passed on the command line
 */
struct ft_arguments {
  const char *target = {};  /**< file path of the config file. */
  unsigned short port = 36443;
  unsigned log_level = 2;        /**< log level */
};

/**
 * Parses the command line options into the arguments struct.
 */
static auto parse_opt(int key, char *arg, struct argp_state *state) -> error_t {
  auto arguments = static_cast<struct ft_arguments *>(state->input);
  switch (key) {
    case 't':
      arguments->target = arg;
      break;
    case 'p':
      arguments->port = static_cast<unsigned short>(strtoul(arg, nullptr, 10));
      break;
    case 'l':
      arguments->log_level = static_cast<unsigned>(strtoul(arg, nullptr, 10));
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, nullptr, doc,
                           nullptr, nullptr,   nullptr};

/**
 * Print the program version in MAJOR.MINOR.PATCH format.
 */
void print_version(FILE *stream, struct argp_state * /*state*/) {
  fprintf(stream, "%s.%s.%s\n", std::to_string(VERSION_MAJOR).c_str(),
          std::to_string(VERSION_MINOR).c_str(),
          std::to_string(VERSION_PATCH).c_str());
}



/**
 *  Main entry point for the program.
 *  
 * @param argc  Command line agument count
 * @param argv  Command line arguments
 * @return 0 on clean exit, -1 on failure
 */
auto main(int argc, char **argv) -> int {
  struct ft_arguments arguments;
  /* Default values */
  arguments.target= "127.0.0.1";

  // Parse the arguments
  argp_parse(&argp, argc, argv, 0, nullptr, &arguments);

  // Set up logging
  std::string ident = "m2-client";
  auto syslog_logger = spdlog::syslog_logger_mt("syslog", ident, LOG_PID | LOG_PERROR | LOG_CONS );

  spdlog::set_level(
      static_cast<spdlog::level::level_enum>(arguments.log_level));
  spdlog::set_pattern("[%H:%M:%S.%f %z] [%^%l%$] [thr %t] %v");

  spdlog::set_default_logger(syslog_logger);
  spdlog::info("M2 client demo starting up");

  try {
    MbmsIfs::M2Client client(arguments.target, arguments.port);

    client.send_setup_request( 
        "901", // MCC
        "56",  // MNC
        0x19B, // eNB ID
        "Demo", // eNB name
        11, // MBSFN Synchronisation Area Id
        {"0", "1", "2"} // MBMS Service Areas
        );
    
  } catch (std::runtime_error ex ) {
    spdlog::error("Exiting on runtime error: {}", ex.what());
  } catch (std::exception ex ) {
    spdlog::error("Exiting on unhandled exception: {}", ex.what());
  }


exit:
  return 0;
}
