cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 11)
#------------------------------------------------------
#set(MODULE Mediator)
#file(GLOB SRC_FILES ${MODULE}/*.cpp)
#link_libraries(pthread)
#add_executable(test${MODULE}  ${SRC_FILES})
#------------------------------------------------------

function(ENTRY arg)
    message("<<<<<<<<<<<<<<<")
    message("arg = ${arg}")
    set(MODULE  ${arg})
    message("MODULE=${MODULE}")

    file(GLOB SRC_FILES ${MODULE}/*.cpp)
    message("SRC_FILES=${SRC_FILES}")

    link_libraries(pthread)
    add_executable(test${MODULE}  ${SRC_FILES})
    message(">>>>>>>>>>>>>>>")
endfunction()
#------------------------------------------------------

ENTRY("Mediator")
ENTRY("Mediator2")
ENTRY("Mediator3")

ENTRY("Decorator")
ENTRY("Decorator2")
ENTRY("Decorator3")
ENTRY("Decorator4")
ENTRY("Decorator5")

ENTRY("Observer")

#ENTRY("Proxy")

ENTRY("State")

ENTRY("Strategy")

ENTRY("Visitor")
ENTRY("Visitor2")
ENTRY("Visitor3")

#------------------------------------------------------
#------------------------------------------------------
#------------------------------------------------------
#------------------------------------------------------
