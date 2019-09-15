cmake_minimum_required (VERSION 3.8)

add_definitions(-DUSE_SND_EXTERNAL_BUFFER)

file(GLOB HEADERS
    
	${CMAKE_CURRENT_SOURCE_DIR}/sound/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/debugger/*.h
)

file(GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/sound/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/debugger/*.cpp
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_executable (Unreal
	${HEADERS}
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/Unreal.rc
)

target_link_libraries(Unreal 
	dinput 
	dxerr8
	
	delayimp
	gdiplus
	kernel32
	user32
	gdi32
	shell32
	comdlg32
	comctl32
	winmm
	setupapi
)

add_custom_command(TARGET Unreal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/cfg
        $<TARGET_FILE_DIR:Unreal>)

add_custom_command(TARGET Unreal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/bass.dll
        $<TARGET_FILE_DIR:Unreal>)