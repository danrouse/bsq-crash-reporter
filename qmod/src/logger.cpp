#include "logger.hpp"

Logger& getLogger() {
    static Logger* logger = new Logger({ID, VERSION});
    return *logger;
}
