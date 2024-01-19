cmake_minimum_required(VERSION 3.10)

project(AudioServer)

find_package(CURL REQUIRED)    
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
    
add_executable(Server cpp/main.cpp)

target_link_libraries(Server PRIVATE CURL::libcurl)
target_link_libraries(Server PRIVATE httplib::httplib)
target_link_libraries(Server PRIVATE nlohmann_json::nlohmann_json)