mark_as_advanced(ZMQ_LIBRARY ZMQ_INCLUDE_DIR)

find_path(ZMQ_INCLUDE_DIR NAMES zmq.h)

find_library(ZMQ_LIBRARY NAMES zmq)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zmq DEFAULT_MSG ZMQ_LIBRARY ZMQ_INCLUDE_DIR)
