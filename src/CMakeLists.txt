add_executable(${PROJECT_NAME}
	Callbacks.cpp
	Gu.cpp
	Input.cpp
	main.cpp
)


target_link_libraries(${PROJECT_NAME} PRIVATE
	stdc++
	pspdebug
	pspctrl
	pspgum
	pspgu
	pspge
	pspdisplay
)

target_compile_options(${PROJECT_NAME} PUBLIC "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.")

create_pbp_file(TARGET ${PROJECT_NAME}
	TITLE "${PROJECT_NAME}" # displayed in XMB
	ICON_PATH "ICON0.PNG"
	# BACKGROUND_PATH PIC1.PNG
)
