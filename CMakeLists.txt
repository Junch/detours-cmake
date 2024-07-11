cmake_minimum_required(VERSION 3.8)

project(DetoursCMake)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT /MP")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

add_subdirectory(3rdparty)
add_subdirectory(simple)
add_subdirectory(dllsample)
add_subdirectory(dllinject)

# Set the destination folder for all binaries in bin/configuration/*
foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${cfg} cfg)
	foreach (trgt simple dllsample dummy runwithdll withdll traceapi syelogd tracessl dllinject dllheap)
		set_target_properties(${trgt} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${cfg} ${CMAKE_CURRENT_BINARY_DIR}/bin/${cfg})
	endforeach()
endforeach()
