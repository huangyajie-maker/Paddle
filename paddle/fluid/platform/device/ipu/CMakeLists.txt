# IPU
IF(WITH_IPU)
  cc_library(ipu_device SRCS device.cc DEPS enforce popart)
  cc_library(ipu_utils SRCS ipu_utils.cc DEPS memory framework_proto popart)
  cc_library(ipu_strategy SRCS ipu_strategy.cc DEPS popart graph framework_proto enforce)
  cc_library(ipu_optimizer SRCS ipu_optimizer.cc DEPS popart enforce)
  cc_library(ipu_executor SRCS ipu_executor.cc DEPS ipu_optimizer ipu_utils popart graph framework_proto)
  cc_library(popart_canonicalization_utils SRCS ${POPART_CANONICALIZATION_SRC} DEPS framework_proto enforce ipu_utils)
  cc_library(ipu_compiler SRCS ipu_compiler.cc DEPS popart graph ipu_utils graph_helper)
  cc_library(ipu_backend SRCS ipu_backend.cc DEPS popart ipu_compiler graph framework_proto enforce ipu_utils ipu_strategy ipu_device ipu_executor graph_helper)
  cc_library(ipu_info SRCS ipu_info.cc DEPS ipu_backend)
ENDIF()
