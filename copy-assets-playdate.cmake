#[[
- This CMake script copies new/modified assets from the ~PROJECT_ROOT/src 
  directory to Playdate's build directories (i.e., Source; PLAYDATE_GAME_NAME.pdx).
  
- Assets are:
  - pdxinfo
  - assets/**
  
- It is intended to run before Playdate's CMake scripts so that these 
  assets are available for its build process as needed.

- Developed against Playdate SDK v2.5.0.
]]

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/src/pdxinfo)
	message(FATAL_ERROR "pdxinfo Playdate Metadata File not found in project's src/; Create it")
	return()
endif()
if (NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/src/assets)
	message(WARNING "assets/ Directory not found in project's src/; No problem (if intended), just letting you know")
	set(PD_ASSETS_MISSING true)
endif()

# pdxinfo game metadata
file(COPY ${CMAKE_CURRENT_LIST_DIR}/src/pdxinfo DESTINATION ../Source)

# assets/
if(NOT PD_ASSETS_MISSING)
	file(COPY ${CMAKE_CURRENT_LIST_DIR}/src/assets DESTINATION ../Source)
	file(COPY ${CMAKE_CURRENT_LIST_DIR}/src/assets DESTINATION ../${PLAYDATE_GAME_NAME}.pdx)
endif()
