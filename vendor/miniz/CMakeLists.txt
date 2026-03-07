 
project(miniz)

 



file(GLOB SOURCES "src/*.c")
 


add_library(miniz STATIC 
    ${SOURCES} 
   
)


target_include_directories(miniz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)