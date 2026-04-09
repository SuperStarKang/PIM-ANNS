cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)

project(faiss_upmem
  VERSION 0.0.1
  DESCRIPTION "upmem support for faiss"
  HOMEPAGE_URL "")




set(HOST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/host)
set(DPU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dpu)
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(UPMEM_ROOT "" CACHE PATH "Root of the UPMEM SDK/backends to use for headers and libraries")

find_program(DPU_COMPILER NAMES dpu-upmem-dpurte-clang REQUIRED)
if(UPMEM_ROOT)
    find_library(DPU_LIBRARY NAMES dpu HINTS "${UPMEM_ROOT}/lib" REQUIRED NO_DEFAULT_PATH)
    find_library(DPUHW_LIBRARY NAMES dpuhw HINTS "${UPMEM_ROOT}/lib" REQUIRED NO_DEFAULT_PATH)
    find_path(UPMEM_INCLUDE_DIR NAMES dpu.h HINTS "${UPMEM_ROOT}/include/dpu" REQUIRED NO_DEFAULT_PATH)
    find_library(PROFILER_LIBRARY NAMES profiler HINTS "${UPMEM_ROOT}/lib" NO_DEFAULT_PATH)
else()
    find_library(DPU_LIBRARY NAMES dpu REQUIRED)
    find_library(DPUHW_LIBRARY NAMES dpuhw REQUIRED)
    find_path(UPMEM_INCLUDE_DIR NAMES dpu.h HINTS /usr/include/dpu REQUIRED)
    find_library(PROFILER_LIBRARY NAMES profiler)
endif()

message(STATUS "Using DPU compiler: ${DPU_COMPILER}")
message(STATUS "Using UPMEM include dir: ${UPMEM_INCLUDE_DIR}")
message(STATUS "Using UPMEM libraries: ${DPU_LIBRARY};${DPUHW_LIBRARY}")



set(FIFO_LINK_OPTION1
    -Wl,--whole-archive ${DPU_LIBRARY} -Wl,--no-whole-archive
)
set(FIFO_LINK_OPTION2
    -Wl,--whole-archive ${DPUHW_LIBRARY} -Wl,--no-whole-archive
)



set(HOST_SOURCES
    ${HOST_DIR}/host_fifo.cpp
    ${HOST_DIR}/util.cpp
)

set(DPU_SOURCES
    ${DPU_DIR}/dpu_kernel.c
)

set(BATCH_DPU_SOURCES
    ${DPU_DIR}/batch_dpu_kernel.c
)

set(DPU_UTIL_SOURCES
    ${DPU_DIR}/cyclecounter.h
    
)

set(COMMON_SOURCES
    ${COMMON_DIR}/common.h
)

add_compile_options(-fPIC)

add_compile_options(-fopenmp)
add_link_options(-fopenmp)


set(DPU_FLAGS -DNR_TASKLETS=16)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb3 -O3")



add_custom_target(batch_dpu_adc_code
    COMMAND ${DPU_COMPILER} -Ofast ${DPU_FLAGS} -o ${CMAKE_SOURCE_DIR}/build/batch_dpu_adc_code  ${BATCH_DPU_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES  ${COMMON_SOURCES} ${DPU_UTIL_SOURCES}
)

add_custom_target(dpu_adc_code
    COMMAND ${DPU_COMPILER} -Ofast ${DPU_FLAGS} -o ${CMAKE_SOURCE_DIR}/build/dpu_adc_code  ${DPU_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES  ${COMMON_SOURCES} ${DPU_UTIL_SOURCES}
)


add_definitions(-DPROJECT_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\")

add_subdirectory(third-party/faiss_upmem)

add_subdirectory(third-party/pybind11)


add_library(dpu_lib STATIC ${HOST_SOURCES})


target_link_libraries(dpu_lib PUBLIC ${FIFO_LINK_OPTION1} ${FIFO_LINK_OPTION2})

target_link_libraries(dpu_lib PUBLIC faiss -lblas -llapack)

target_link_libraries(dpu_lib PUBLIC boost_coroutine)
if(PROFILER_LIBRARY)
    target_link_libraries(dpu_lib PUBLIC ${PROFILER_LIBRARY})
endif()

target_include_directories(dpu_lib PUBLIC ${HOST_DIR})
target_include_directories(dpu_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(dpu_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/faiss_upmem/ )
target_include_directories(dpu_lib PUBLIC ${UPMEM_INCLUDE_DIR})



target_sources(dpu_lib PUBLIC ${COMMON_SOURCES})

add_dependencies(dpu_lib dpu_adc_code)


add_executable(main EXCLUDE_FROM_ALL main.cpp)
target_link_libraries(main PUBLIC dpu_lib)

add_executable(train_msmarco_ivfpq EXCLUDE_FROM_ALL utils/train_msmarco_ivfpq.cpp)
target_link_libraries(train_msmarco_ivfpq PRIVATE faiss -lblas -llapack)
target_include_directories(train_msmarco_ivfpq PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(train_msmarco_ivfpq PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/faiss_upmem/)
