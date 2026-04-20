if(NOT NCCL_OS_WINDOWS)
  set(NCCL_AGENT_REMOTE_HOST "192.168.3.3" CACHE STRING "Default remote IPv4 host for nccl-agent")
  set(NCCL_AGENT_REMOTE_PORT "19090" CACHE STRING "Default remote UDP port for nccl-agent")

  add_executable(nccl-agent nccl_agent.cc)
  target_include_directories(nccl-agent PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
  target_compile_definitions(nccl-agent PRIVATE
    NCCL_AGENT_FORWARD_HOST="${NCCL_AGENT_REMOTE_HOST}"
    NCCL_AGENT_FORWARD_PORT=${NCCL_AGENT_REMOTE_PORT}
  )
  target_link_libraries(nccl-agent PRIVATE Threads::Threads)
  set_target_properties(nccl-agent PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()
