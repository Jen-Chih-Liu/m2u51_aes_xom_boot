# CMakeList.txt : CMake project for nu_com_isp_sboot, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("nu_com_isp_sboot")

# Add source to this project's executable.
add_executable (nu_com_isp_sboot "nu_com_isp_sboot.c" "aes.c" "aes.h")

target_compile_definitions(nu_com_isp_sboot PRIVATE CBC=1 ECB=0 CTR=0)


target_include_directories(nu_com_isp_sboot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET nu_com_isp_sboot PROPERTY C_STANDARD 99)
endif()
